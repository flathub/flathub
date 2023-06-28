#! /usr/bin/env python

# A simple cross-platform program to copy all entities from a .map, strip out
# any brush data, and write out a file with the resulting data.

# Usage: copy_entites.py [-i infile] [-o outfile] 
#   If "infile" is not specified, it is assumed to be stdin.
#   If "outfile" is not specified, it is assumed to be stdout.

# This needs to be run with Python 2.x; it is not compatible with 3.x!

import getopt, sys, re

num_brush_models = 1


def usage ():
    print (
"""Usage: copy_entites.py [-i infile] [-o outfile] 
    If "infile" is not specified, it is assumed to be stdin.
    If "outfile" is not specified, it is assumed to be stdout.""")


def get_filenames ():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "i:o:")
    except getopt.GetoptError, err:
        print str(err)
        usage ()
        sys.exit(2)
    
    infile_name = None
    outfile_name = None
    
    for option, arg in opts:
        if option == "-i":
            infile_name = arg
        elif option == "-o":
            outfile_name = arg
        else:
            assert False, "unhandled option (can't happen!)"
    
    return infile_name, outfile_name


#get the brush's origin given all its component tokens
#return as [x,y,z] origin
#the origin is the average of the maxs and mins of the brush
def get_brush_origin (brush):
    brush_len = len(brush)
    mins = [int(brush[1]), int(brush[2]), int(brush[3])]
    maxs = [int(brush[1]), int(brush[2]), int(brush[3])]
    
    vec_offset = 6 #6 to skip first vector, which we already took care of
    while vec_offset+22 < brush_len: #loop accross all lines
        while vec_offset%24 < 10: #loop accross all vectors on a line
            for axisNum in range(3): #loop accross each axis of current vector
                axis = int(brush[vec_offset])
                if mins[axisNum] > axis:
                    mins[axisNum] = axis
                if maxs[axisNum] < axis:
                    maxs[axisNum] = axis
                vec_offset += 1 #next axis
            vec_offset += 2 #skip the " ) ( " tokens to next vector
        vec_offset += 24 #skip to same offset on the next line
        vec_offset -= (vec_offset % 24)-1 #vec_offset is now (line_num-1)*24+1
    
    origin = [str(int(float(mins[i]+maxs[i])/2.0)) for i in range(3)]
    return origin


#parse brush data starting at the indexth token
#return the new index, the brush's "contents" bitmask, and the brush's origin
#if the brush's "contents" has the CONTENTS_ORIGIN bit set
#TODO: better validation?

#NOTE: Each face of the brush is specified in the following format:
# ( x y z ) ( x y z ) ( x y z ) texture_name xoffs yoffs xscale yscale
#Additionally, other numbers may follow, the first of which is the brush
#contents bitmask. If the brush contents bitmask is zero, and if no other 
#properties need to be specified for that face, then it may be omitted. 
#Otherwise it must be there and it must be the same on every face of the 
#brush. 
def parse_brush (tokens, num_tokens, index):
    brush = []
    while index < num_tokens:
        if tokens[index] == "}":
            index += 1
            assert len(brush) > 21, "Invalid brush!"
            if brush[21] in ('(', '}'):
                return index, 0, None #Contents are 0 and have been omitted.
            contents = int(brush[21]) #Brush contents are token 21. I counted.
            if contents & 16777216: #CONTENTS_ORIGIN
                origin = get_brush_origin (brush)
            else:
                origin = None
            return index, contents, origin
        brush += [tokens[index]]
        index += 1
    assert False, "reached end of file without finishing out an entity's brush data!"


#parse an entity starting at the indexth token
#return the entity and a new index
def parse_ent (tokens, num_tokens, index):
    global num_brush_models
    ent = {}
    
    if tokens[index] != "{":
        assert False, "unexpected token "+tokens[index]+" (expected '{'!)"
    
    index += 1
    
    key = None
    while index < num_tokens:
        t = tokens[index]
        if t == "}":
            assert (not key), "reached end of entity when expecting a value for key "+str(key)
            if '"classname"' in ent and ent['"classname"'] == '"worldspawn"':
                if '"model"' in ent:
                    num_brush_models -= 1
                    del(ent['"model"'])
            return ent, index+1

        elif t == "{":
            #entity has a brush model
            if not '"model"' in ent: 
                #if this is the first brush we've encountered, set the "model"
                #key and increment the global counter of brush models
                ent['"model"'] = '"*'+str(num_brush_models)+'"'
                num_brush_models += 1
            index, brush_contents, brush_origin = parse_brush (tokens, num_tokens, index+1)
            if brush_contents & 16777216: #CONTENTS_ORIGIN
                #entity's origin needs to come from this brush
                ent['"origin"'] = '"'+brush_origin[0]+' '+brush_origin[1]+' '+brush_origin[2]+'"'

        elif key: #if "key" has already been set, we are expecting a value
            val = t
            index += 1
            while val.find('"') == val.rfind('"'): #for values with whitespace in them
                assert index < num_tokens, "reached end of file in the middle of a value!"
                assert tokens[index] != "}", "reached end of entity in the middle of a value!"
                val += ' '+tokens[index]
                index += 1
            ent[key] = val
            key = None

        else:
            key = t
            index += 1
            while key.find('"') == key.rfind('"'): #for keys with whitespace in them
                assert index < num_tokens, "reached end of file in the middle of a key!"
                assert tokens[index] != "}", "reached end of entity in the middle of a key!"
                key += ' '+tokens[index]
                index += 1
    
    assert False, "reached end of file without finishing out an entity!"


def parse_file (infile):
    tokens = re.sub("//.*\n", "", infile.read()).split()
    
    ents = []
    
    i = 0
    l = len(tokens)
    while i < l:
        ent, i = parse_ent (tokens, l, i)
        ents += [ent]
    
    return ents


def write_ent_list (outfile, ent_list):
    for ent in ent_list:
        outfile.write ("{\n")
        for (k,v) in ent.iteritems():
            outfile.write ("\t"+str(k)+"\t\t"+str(v)+"\n")
        outfile.write ("}\n")


def main ():
    infile_name, outfile_name = get_filenames()
    
    if infile_name == None:
        infile = sys.stdin
    else:
        try:
            infile = open (infile_name, 'r')
        except IOError, err:
            print str(err)
            usage ()
            sys.exit (2)
    
    if outfile_name == None:
        outfile = sys.stdout
    else:
        try:
            outfile = open (outfile_name, 'w')
        except IOError, err:
            print str(err)
            usage ()
            sys.exit (2)

    ent_list = parse_file (infile)
    write_ent_list (outfile, ent_list)



if __name__ == "__main__":
    main()
