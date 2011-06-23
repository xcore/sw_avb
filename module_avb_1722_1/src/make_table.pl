# This script constructs a compact tree for parsing the 1722.1
# SEC 'address' words. The words are described in the 1722.1 document
# but a simple breakdown is:
#
# 2 bits constant 10b
# 2 bits section
# 2 bits subsection
# 3 bits subsubsection
# 1 bit 'has item' flag
# 6 bits subaddress
# 16 bits item number
# 
# The idea is that the section, subsection, subsubsection and subaddress
# indicate the data, and the item number represents the instance of something
# when there is more than one unit possessing the data.  ie. 'Unit name' does
# not have 'has item' flag set or an item number, but 'Stream name' would
# because there is more than one stream.
#
# Section 00b is a special 'meta' section where the subsection and subsubsection
# bits are unused, and the 'address' can affect the following 'addresses'. Ie
# there may be an address called 'Read at a given time', which means that the
# following address reads must be delayed until that time.
#
# Address really means an ID for a given piece of data, eg 'name', 'id', 'volume
# level', etc.  
#
#
# In this implementation of the parse tree there are 2 entry types:
#
# 1) a 32 bit non-leaf node
# 2) a 32 bit leaf node
#
# The non-leaf node looks like:
#   0x80000000 - Children are leaf nodes
#   0x40000000 - Last sibling
#   0x30000000 - Address word index bit selector *
#   0x0FC00000 - Count of children **
#   0x003F0000 - Value of first child **
#   0x0000FFFF - Offset to next sibling
#   
# * 0=Section+Subsection, 1=Subsubsection, 2=Subaddress, 3=Item (for meta)
# ** Only valid for 'children are leaf nodes' entries
#
# Leaf nodes are:
#   0x80000000 - Flag to say if the pointer is to a data block or a function returning data
#   0x40000000 - Flag to say if the function is read/write (read only otherwise)
#   0x3F000000 - Enumeration value for the type of data
#   0x00FFFFFF - Index of data in data block or index of function in function table
#
#
# The input to this parser is a file of lines containing comma separated fields:
#    SEC address value
#    Data type
#    Read only ('ro' / 'rw' / 'wo')
#    Function/Data bit ('f' / 'd')
#    Data (numbers as parsable by perl), or function data name
#    (Optional trailing # followed by comment)
# 
# Lines starting '#' and blank lines are ignored. 
#
# The output is:
#    A function table of the functions to get dynamic data
#    A Static block of data for the non-dynamic data
#    A block of tree nodes

use strict;
use Data::Dumper;

# Known types - array contains [enumeration_value, size, size_is_variable_flag]
my %types = (
  'gtptime' => [0, 8, 'fixed'],
  'int32' => [1, 4, 'fixed'],
  'maskcompare' => [2, 8, 'fixed'],
  'eui64' => [3, 8, 'fixed'],
  'string' => [4, 0, 'variable'],
  'oui' => [5, 6, 'fixed'],
  'int64' => [6, 8, 'fixed'],
  'macaddr' => [7, 6, 'fixed'],
  'int16' => [8, 2, 'fixed'],
  'bool' => [9, 1, 'fixed'],
  '4meter' => [10, 4, 'fixed'],
  'gaindb' => [11, 4, 'fixed']
);

# Names of the bit fields in the address word
my @field_names = (
 'section/subsection',
 'subsubsection',
 'subaddress',
 'item'
);

#
# Functions
#

sub parse_file {
  my ( $tree ) = @_;
  
  while (<>)
  {
    chomp;
    next if /^\w*$/;
    next if /^#.*/;
  
    # Pull records from file
    my ($address, $datatype, $readonlyp, $funcp, $dataval, $comment) = split '\W*,\W*|\W*#\W*';
  
    # Generate record and numerical address
    my $record = [ $datatype, $readonlyp, $funcp, $dataval, $comment ];
    my $addr = hex $address;
  
    # Pull fields from address word
    my $i1 = (($addr & 0x3C000000) >> 26) . " " . 0;
    my $i2 = (($addr & 0x03800000) >> 23) . " " . 1;
    my $i3 = (($addr & 0x003F0000) >> 16) . " " . 2;
    my $i4 = (($addr & 0x0000FFFF) >> 0)  . " " . 3;

    # Store only those fields appropriate to the SEC 'form'
    if ($i1 == 0) {
      $tree->{$i1}{$i4} = $record;
    } else {
      $tree->{$i1}{$i2}{$i3} = $record;
    }
  }
}

sub process_tree {
  my ( $hash, $list ) = @_;

  my @keys = sort { $a <=> $b } keys %{$hash};
  
  while (scalar @keys > 0) {
    my $k1 = shift @keys;
    my ($val, $level) = split ' ', $k1;

    # Push a placeholder into the output and take a reference to it
    push @{$list}, [];
    my $listentry = \$list->[-1];
    
    my $leaf = ((ref $hash->{$k1}) ne "HASH") ? 1 : 0;
    
    # Potentially combine some of the leaf elements together
    my $combine = 1;
    if ($leaf == 1) {
      push @{$list}, [ 1, @{$hash->{$k1}}];
      while ( (scalar @keys != 0) and (@keys[0] == $k1+1) ) {
         $k1 = shift @keys;
         $combine = $combine + 1;
         push @{$list}, [ 1, @{$hash->{$k1}}]; 
      }
    } else {
      process_tree( $hash->{$k1}, $list );
    }
        
    my $last = (scalar @keys == 0) ? 1 : 0;
    
    # Update the reference with the correct forward index
    ${$listentry} = [0, $val, $level, scalar ( @{$list} ), $combine, $leaf, $last];
  }
}

sub build_data {
  my ( $list, $func, $data ) = @_;

  my $data_offset = 0;

  foreach my $e (@{$list}) {
    #if (undef $types{$e->[1]}) {
    #  print "Error: unknown type information: \"$e->[5]\"";
    #  die;
    #}
    if ($e->[0] == 1) {
      if ($e->[3] eq 'f') {
        my $func_entry = scalar(@{$func});
        push @{$func}, "avb_1722_1_getset_".$e->[4];
        $e->[4] = $func_entry;
        $e->[3] = 1;
        $e->[2] = ($e->[2] eq 'rw') ? 1 : 0;
        $e->[1] = $types{$e->[1]}->[0];
      } else {
        if ($e->[2] eq 'rw') {
          print "Error: cannot have a writable data in constant data block: \"$e->[5]\"";
          die;
        }
        # Put a record on the data block description [offset,size,data]
        my $type = $types{$e->[1]};
        my $len = $type->[1];
        my $value = [];
        if ($e->[1] eq 'string') {
          $len = (length $e->[4])+1;
          $value = [(map { sprintf "0x%02x", ord } split //, $e->[4]), '0x00'];
        } else {
          my $n = oct $e->[4];
          for (my $i = 0; $i<$len; $i++) {
            push @{$value}, (sprintf "0x%02x",($n&0xff));
            $n >>= 8;
          }
        }
        push @{$data}, [$data_offset,$len,$value,"$e->[5] \"$e->[4]\""];
        $e->[4] = $data_offset;
        $e->[3] = 0;
        $e->[2] = ($e->[2] eq 'rw') ? 1 : 0;
        $e->[1] = $types{$e->[1]}->[0];
        $data_offset += $len;
      }
    }
  }  
}


#
# MAIN
#

my %tree;

parse_file( \%tree );

# Turn the tree into a list
my @list;
process_tree( \%tree, \@list );

# Build a dispatch table for the functional entries and a data block for the data ones
my @function_table;
my @data_block;
build_data( \@list, \@function_table, \@data_block ); 

#
# Print out results
#

print "// AVB 1722.1 SEC Parse Table\n//\n// This file was autogrenerated by the make_table.pl script.\n\n\n";


# Print out parse tree
print "unsigned int avb_1722_1_sec_parse_tree[] = {\n";
foreach my $e (@list) {
  if ($e->[0] == 0) {
    my $v = ($e->[5]<<31) + ($e->[6]<<30) + ($e->[2]<<28) + ($e->[4]<<22) + ($e->[1]<<16) + ($e->[3]<<0);
    printf "  0x%08x, // compare %s to range (0x%x, 0x%x)\n", $v, $field_names[$e->[2]], $e->[1], $e->[1]+$e->[4]-1;
  } else {
    my $v = ($e->[3]<<31) + ($e->[2]<<30) + ($e->[1]<<24) + ($e->[4]<<0);
    printf "  0x%08x, // %s (%s at 0x%x)\n", $v, $e->[5], ($e->[3] ? "Function" : "Data"), $e->[4];
  }
}
print "};\n\n";

# Print function pre-declaractions
my $n=0;
foreach my $e (@function_table) {
  print "extern unsigned int $e(char* address, unsigned set, char *data);\n";
  $n++;
}
print "\n\n";

# Print function table
print "unsigned avb_1722_1_sec_dispatch(unsigned func_num, char* address, unsigned set, char* data)\n{\n";
print "  switch (func_num) {\n";

my $n=0;
foreach my $e (@function_table) {
  print "  case $n:\n    return $e(address, set, data);\n";
  $n++;
}
print "  }  return 0;\n}\n\n";

# Print enumeration types
print "typedef enum {\n";
foreach my $e (keys %types) {
  print "  AVB_1722_1_SEC_DATA_TYPE_",uc $e," = ",$types{$e}->[0],",\n";
}
print "} avb_1722_1_sec_data_type_t;\n\n";

# Print the data block
print "unsigned char avb_1722_1_sec_constant_data[] = {\n";
foreach my $e (@data_block) {
  print "  ",join(',', @{$e->[2]}),",\t// $e->[3]\n";
}
print "};\n\n";

# Print the table of lengths of each type
print "unsigned int avb_1722_1_sec_data_type_length_t[] = {\n";
foreach my $e (keys %types) {
  print "  ",$types{$e}->[1],", //",$e,"\n";
}
print "};\n\n";




