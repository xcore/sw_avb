import sys
import re
import string
import os

string_regex = re.compile(r'"[^"]*"')

def convert_string_to_char_array(str):
    s = []
    l = list(str)
    l = l[:64]
    for c in l:
        s.append("\'" + c + "\',")
    if len(l) < 64:
        s.append("'\\0',")
        i = 0
        while (i < (62 - len(l))):
            s.append("0,")
            i += 1
        s.append("0")
    return ''.join(s)


def do_replace(read_file, write_file, replace_defines):
    write_file.write("/************************************************************************/\n")
    write_file.write("/* File generated from " + read_file.name + ". DO NOT MODIFY THIS FILE. */ \n")
    write_file.write("/************************************************************************/\n")

    for line in read_file:
        if not line.startswith("#include") and len(string_regex.findall(line)) == 1:  # Look for only one string per line
            modified_line = line
            for str in string_regex.findall(line):
                if replace_defines == 1:
                    modified_line = modified_line.replace(str, '')  # Strip the quoted string
                    modified_line = modified_line.rstrip('\r\n')
                str = str.replace("\"", '')  # Strip the quotes
                s = convert_string_to_char_array(str)
                if len(s) != 0:
                    if replace_defines == 1:
                        write_file.write(modified_line + s + '\n')
                    else:
                        write_file.write('  ' + s + ',\n')
        else:
            write_file.write(line)


def main():
    srcpath = sys.argv[1]
    dstpath = sys.argv[2]
    read_file = open(os.path.join(srcpath, 'aem_descriptors.h.in'), 'r')
    write_file = open(os.path.join(dstpath, 'aem_descriptors.h'), 'w')

    do_replace(read_file, write_file, 0)

    read_file = open(os.path.join(srcpath, 'aem_entity_strings.h.in'), 'r')
    write_file = open(os.path.join(dstpath, 'aem_entity_strings.h'), 'w')

    do_replace(read_file, write_file, 1)

    print "AEM descriptor header file generation complete"

main()
