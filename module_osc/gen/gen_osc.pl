
use Data::Dumper;
use Switch;
$target_dir = shift(@ARGV);
@files = @ARGV;
%schema = ();
open($osc_tree_h, ">$target_dir/osc_tree.h");
open($osc_tree_c, ">$target_dir/osc_tree.c");
open($osc_api_h,  ">$target_dir/avb_api.h");



sub parseType {
    my @types = ();
    my ($typestring) = @_;
    my @xs = ();
    $match = 1;
    while ($match) {
        if ($typestring =~ /^(s\{[^\}]*\})(.*)$/ ||
            $typestring =~ /^(auc)(.*)$/ ||
            $typestring =~ /^(i|h|B|s|a)(.*)$/) {
            push(@xs, $1);
            $typestring = $2;
            $match = 1;
        }
        else {
            $match = 0;
        }
    }

    foreach my $type (@xs) {
        if ($type =~ /^i/) {   
            push(@types, 'WORD');
        }
        if ($type =~ /^B/) {
            push(@types, 'BOOL');
        }
        if ($type =~ /^h/) {     
            push(@types, 'LONGWORD');
        }
        if ($type =~ /^s\{(.*)\}/) {        
            push(@types, 'ENUM:' . $1);
        }
        if ($type =~ /^s$/) {            
            push(@types, 'STRING');
        }
        if ($type =~ /^a$/) {      
            push(@types, 'WORDARRAY');
        }
        if ($type =~ /^auc$/) {            
            push(@types, 'UCHARARRAY');
        }
    }

    \@types;
}

sub readTree {
    foreach my $fname (@files) {
        print "Processing $fname\n";
        open(my $osc_file, "<$fname");
        while (<$osc_file>) {
            chomp($_);
            @fields = split(/\s+/,$_);
            @path = split(/\//,$fields[0]);
            shift(@path);
            $type = $fields[1];
            $rw = $fields[2];
            
            $hash = \%schema;
            foreach $key (@path) {
                if (defined $$hash{$key}) {
                    $hash = $$hash{$key};
                }
                else {
                    my %subtree = ();
                    $$hash{"$key"} = \%subtree;
                    $hash = \%subtree;
                }
            }
            $$hash{'__type'} = parseType($type);
            $$hash{'__rw'} = $rw;
        }
        close ($osc_file);
    }
}


sub enumName 
{
    my $enum = "OSC_NODE";
    foreach my $name (@_) {
        $ucname = uc($name);
        if ($name eq "#") {
            $enum .= "_HASH";
        }
        else {
            $enum .= "_$ucname";
        }
    }
    $enum;
}



sub get_param
{
    my ($type, $index) = @_;
       
    switch ($type) {
        case "WORD" { $param = "REFERENCE_PARAM(int, a$index)";}
        case /ENUM/ { $param = "REFERENCE_PARAM($enum_name, a$index)";}
        case "BOOL" { $param = "REFERENCE_PARAM(int, a$index)";}
        case "LONGWORD" { $param = "unsigned int a${index}[2]";}
        case "STRING" { $param = "char a${index}[]";}
        case "WORDARRAY" { $param = "int a${index}[], REFERENCE_PARAM(int, a${index}_len)";}
        case "UCHARARRAY" { $param = "unsigned char a${index}[], REFERENCE_PARAM(int, a${index}_len)";}
    }

    $param
}

sub set_param
{
    my ($type, $index) = @_;
       
    switch ($type) {
        case "WORD" { $param = "int a$index";}
        case /ENUM/ { $param = "$enum_name a$index";}
        case "BOOL" { $param = "int a$index";}
        case "LONGWORD" { $param = "unsigned int a${index}[2]";}
        case "STRING" { $param = "char a${index}[]";}
        case "WORDARRAY" { $param = "int a${index}[], int a${index}_len";}
        case "UCHARARRAY" { $param = "unsigned char a${index}[], int a${index}_len";}
    }

    $param
}

sub get_arg
{
    my ($type, $index) = @_;    

    switch ($type) {
        case "WORD" { $arg = "a$index";}
        case /ENUM/ { $arg = "a$index";}
        case "BOOL" { $arg = "a$index";}
        case "LONGWORD" { $arg = "a${index}";}
        case "STRING" { $arg = "a${index}";}
        case "WORDARRAY" { $arg = "a${index}, a${index}_len";}
        case "UCHARARRAY" { $arg = "a${index}, a${index}_len";}
    }

    $arg

}

sub set_arg
{
    my ($type, $index) = @_;    

    switch ($type) {
        case "WORD" { $arg = "REFERENCE_TO a$index";}
        case /ENUM/ { $arg = "REFERENCE_TO a$index";}
        case "BOOL" { $arg = "REFERENCE_TO a$index";}
        case "LONGWORD" { $arg = "a${index}";}
        case "STRING" { $arg = "a${index}";}
        case "WORDARRAY" { $arg = "a${index}, REFERENCE_TO a${index}_len";}
        case "UCHARARRAY" { $arg = "a${index}, REFERENCE_TO a${index}_len";}
    }

    $arg

}

sub get_boxed_arg
{
    my ($type, $index) = @_;    

    switch ($type) {
        case "WORD" { $arg = "&a[$index].val.word";}
        case /ENUM/ { $arg = "($enum_name *) &a[$index].val.word";}
        case "BOOL" { $arg = "&a[$index].val.word";}
        case "LONGWORD" { $arg = "(unsigned int *) a[$index].val.word_array";}
        case "STRING" { $arg = "a[$index].val.str";}
        case "WORDARRAY" { $arg = "a[$index].val.word_array, &a[$index].len";}
        case "UCHARARRAY" { $arg = "a[$index].val.byte_array, &a[$index].len";}
    }

    $arg
}

sub osc_type
{
    my ($type) = @_;    

    switch ($type) {
        case "WORD" { $osc_type = "OSC_WORD";}
        case /ENUM/ { $osc_type = "OSC_WORD";}
        case "BOOL" { $osc_type = "OSC_WORD";}
        case "LONGWORD" { $osc_type = "OSC_LONGWORD";}
        case "STRING" { $osc_type = "OSC_STRING";}
        case "WORDARRAY" { $osc_type = "OSC_WORD_ARRAY";}
        case "UCHARARRAY" { $osc_type = "OSC_BYTE_ARRAY";}
    }

    $osc_type    
}

sub makeEnum {
    my ($f, $choices) = @_;
    $enum = "#ifndef __". uc($f) ."_T__\n";
    $enum .= "enum ${f}_t \n{\n";
    @c = split(/\|/,$choices);
    foreach my $choice (@c) {
        $choice = uc($f) . "_" . uc($choice);
        $enum .= "$choice,\n";
    }
    $enum .= "};\n#endif\n";

    $enums .= $enum;
}

sub makeEnums {
    (my $f, my $type) = @_;
    foreach $t (@$type) {
        if ($t =~ /ENUM:(.*)/) {
            makeEnum($f, $1);
        }
    }

}

sub getFunc
{
    (my $path, my $type, my $rw, my $enum) = @_;
    my $fname = "";
    my $first = 1;
    my $n = 0;
    my $arg0 = "";
    my $inlines = "";
    my $froot = "";
    my $cases = "";

    if ($rw eq "rw") {
        $fname = "getset";
        $arg0="int set, ";
    }
    if ($rw eq "w") {
        $fname = "set";
    }
    if ($rw eq "r") {
        $fname = "get";
    }

    my $x = 0;
    my @hash_params = ();
    my @hash_args = ();
    my @boxed_hash_args = ();
    foreach my $name (@$path) {            
        if ($name eq "#") {
            push(@hash_params,"int h$n");
            push(@hash_args,"h$n");
            push(@boxed_hash_args,"h[$n]");
        }
        
        else {
            if ($x != 0) {
                $froot .= "_";
            }
            $froot .= "$name";
            $x = 1;
        }
    }

    $fname = $fname .= "_$froot";

    $enum_name = "enum ${froot}_t";

    makeEnums($froot, $type);

    my @set_params = ();
    my @get_params = ();
    my @set_args = ();
    my @get_args = ();
    my @get_boxed_args = ();
    $i = 0;
    for $t (@$type) {
        push(@set_params,set_param($t,$i));
        push(@get_params,get_param($t,$i));
        push(@set_args,set_arg($t,$i));
        push(@get_args,get_arg($t,$i));
        push(@get_boxed_args,get_boxed_arg($t, $i));
        $i++;
    }

    @set_params = (@hash_params , @set_params);
    @get_params = (@hash_params , @get_params);
    @set_args = (@hash_args , @set_args);
    @get_args = (@hash_args , @get_args);
    @get_boxed_args = (@boxed_hash_args, @get_boxed_args);
       



    $set_params = join(",",@set_params);
    $get_params = join(",",@get_params);
    
    $set_args = join(",",@set_args);
    $get_args = join(",",@get_args);
    $get_boxed_args = join(",",@get_boxed_args);

    $case =  "    case $enum:\n       get_${froot}($get_boxed_args);break;\n";

    if ($rw eq "rw") {
        $inlines .= "\ninline int set_${froot}($set_params)\n{return getset_${froot}(1, $set_args);}\n";
        $inlines .= "\ninline int get_${froot}($get_params)\n {return getset_${froot}(0, $get_args);}\n\n";
    }

    
    $args = $arg0 . $get_params;
    if ($rw eq "rw") {
        $n++;
    }
    

    ($args, $fname, $inlines, $bcase);   
}




sub nodeName 
{
    my $n = "osc_node";
    foreach my $name (@_) {
        if ($name eq "#") {
            $n .= "_hash";
        }
        else {
            $n .= "_$name";
        }
    }
    $n;
}

sub keyName
{
    my ($key) = @_;
    my $keyname;
    if ($key eq "#") {
        $keyname = "osc_name_hash";
    }
    else {
        $keyname = "osc_name_$key";
    }
    $keyname;        
}

sub createNames0
{
    my ($hash,$func) = @_;
    foreach $key (keys %{$hash}) {
        if ($key !~ /^__*/) {
            $keyname = keyName($key);
            if (!(defined $names{$keyname})) {
                $names{$keyname} = $key;
            }
            createNames0($$hash{$key});
        }
    }
}

sub createNames 
{
    %names = ();
    createNames0(\%schema);
    foreach my $name (keys %names) {
        if ($key !~ /^__*/) {
            $len = length($names{$name})+1;
            print $osc_tree_c "char ${name}[$len] = \"$names{$name}\";\n";
        }
    }
}

sub createEnums
{
    my ($hash,$func) = @_;
    foreach $key (keys %{$hash}) {
        if ($key !~ /^__*/) {
            push(@path, $key);
            my $enum = enumName(@path);
            print $osc_tree_h "  $enum,\n";       
            createEnums($$hash{$key});
            pop(@path);
        }
    }
}



sub createNodes
{
    my ($hash,$func) = @_;
    my $sibling = "NULL"; 
    my $parent = "NULL";
    foreach $key (keys %{$hash}) {
        if ($key !~ /^__*/) {
            push(@path, $key);            
            my @ppath = @path;
            my $keyname = keyName($key);
            my $node = nodeName(@path);
            my $enum = enumName(@path);            
            my $child = "NULL";
            pop(@ppath);
            if (scalar(@ppath) != 0) {
                $parent = "&" . nodeName(@ppath);
            }
            foreach $c (keys %{$$hash{$key}}) {
                if ($c !~ /^__*/) {
                    my @cpath = @path;
                    push(@cpath,$c);
                    my $childname = nodeName(@cpath);
                    $child = "&$childname";
                }
            }
            
            my @ntype = ();
            if (defined ${$$hash{$key}}{'__type'}) {
                my $type = ${$$hash{$key}}{'__type'};
                for $t (@$type) {
                    push(@ntype, osc_type($t));
                }
            }
            $ntype = "{" . join(",",@ntype) . "}";

            print $osc_tree_h "extern osc_node $node;\n";
            print $osc_tree_c "osc_node $node = {\n";
            print $osc_tree_c "   $enum,\n";
            print $osc_tree_c "   $keyname,\n";
            print $osc_tree_c "   $parent,\n";
            print $osc_tree_c "   $child,\n";
            print $osc_tree_c "   $sibling,\n";
            print $osc_tree_c "   $ntype\n";
            print $osc_tree_c  "};\n\n";           
     

       
            if (defined ${$$hash{$key}}{'__type'}) {
                my $type = ${$$hash{$key}}{'__type'};
                my $rw = ${$$hash{$key}}{'__rw'};
                (my $args, my $func, my $inlines) = 
                    getFunc(\@path, $type, $rw, $enum, $case);
                
                $funcs .= "int ${func}($args);\n$inlines";
                
                $cases .= $case;
            }

            createNodes($$hash{$key});
            pop(@path);
            $sibling = "&$node";
        }
    }
}

sub createRoot{
    my ($hash,$func) = @_;
    my $child = "NULL";
    foreach $c (keys %{$hash}) {
        if ($c !~ /^__*/) {
            my @cpath = @path;
            push(@cpath,$c);
            my $childname = nodeName(@cpath);
            $child = "&$childname";
        }
    }
    print $osc_tree_c "osc_node *osc_root = $child;\n\n";
}

sub Main {
    print "Creating osc code\n";
    print $osc_tree_h "// Autogenerated OSC data file: osc_tree.h\n";
    print $osc_tree_h "//\n";
    print $osc_tree_h "// This file was generated by the gen_osc.pl script from the following files:\n";
    print $osc_tree_h "//    ".join("\n//    ", @files)."\n";
    print $osc_tree_h "//\n";
    print $osc_tree_h "// See the module_osc README file for more details\n";
    print $osc_tree_h "//\n";
    print $osc_tree_h "#ifndef _osc_tree_h_\n";
    print $osc_tree_h "#define _osc_tree_h_\n"; 

    print $osc_tree_c "// Autogenerated OSC data file: osc_tree.c\n";
    print $osc_tree_c "//\n";
    print $osc_tree_c "// This file was generated by the gen_osc.pl script from the following files:\n";
    print $osc_tree_c "//    ".join("\n//    ", @files)."\n";
    print $osc_tree_c "//\n";
    print $osc_tree_c "// See the module_osc README file for more details\n";
    print $osc_tree_c "//\n";
    print $osc_tree_c "#include \"osc_types.h\"\n";
    print $osc_tree_c "#define __OSC_IMPL\n";
    print $osc_tree_c "#include \"avb_api.h\"\n";
    print $osc_tree_c "#include \"osc_tree.h\"\n";
    print $osc_tree_c "#include <stdlib.h>\n";
    print $osc_tree_c "#include <xccompat.h>\n";
    print $osc_tree_c "\n\n#ifdef __XC__\n#define REFERENCE_TO\n#else\n#define REFERENCE_TO &\n#endif\n\n";

    print $osc_api_h "// Autogenerated OSC data file: avb_api.h\n";
    print $osc_api_h "//\n";
    print $osc_api_h "// This file was generated by the gen_osc.pl script from the following files:\n";
    print $osc_api_h "//    ".join("\n//    ", @files)."\n";
    print $osc_api_h "//\n";
    print $osc_api_h "// See the module_osc README file for more details\n";
    print $osc_api_h "//\n";
    print $osc_api_h "#ifndef _avb_api_h_\n";
    print $osc_api_h "#define _avb_api_h_\n"; 
    print $osc_api_h "#include <xccompat.h>\n";  
    print $osc_api_h "\n\n#ifdef __XC__\n#define REFERENCE_TO\n#else\n#define REFERENCE_TO &\n#endif\n\n";

    readTree();
    @path = ();

    print $osc_tree_h "enum osc_node_type {\n";
    createEnums(\%schema);
    print $osc_tree_h "};\n\n";


    createNames();
    print $osc_tree_h "\n\n";
    print $osc_tree_c "\n\n";

    createNodes(\%schema);
    print $osc_tree_h "\n\n";
    print $osc_tree_c "\n\n";

    print $osc_api_h "$enums\n\n";
    print $osc_api_h "#ifndef __OSC_IMPL\n\n";
    print $osc_api_h "$funcs";
    print $osc_api_h "\n#endif // __OSC_IMPL\n";
    print $osc_api_h "\n\n";

    $funcs =~ s/inline //g;
    
    print $osc_tree_c "$funcs";
    print $osc_tree_c "\n\n";

    print $osc_tree_c "

void osc_get(osc_node *n, int h[], osc_val a[]) {
  switch (n->id) {
$cases
  }
}

";

    createRoot(\%schema);
    print $osc_tree_c "\n\n";

    print $osc_tree_h "#endif // _osc_tree_h_\n";
    print $osc_api_h "#endif // _avb_api_h_\n";
    close($osc_tree_h);
    close($osc_tree_c);
    close($osc_api_h);

    print "Created $target_dir/osc_tree.h\n";
    print "Created $target_dir/osc_tree.c\n";
    print "Created $target_dir/avb_api.h\n";
}

Main();

#print Dumper(%schema);


