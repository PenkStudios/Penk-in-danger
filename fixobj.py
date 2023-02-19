"""
So here is the thing. 
---------------------

Raylib crashed after loading MY .obj file (exported from blender).
BTW I use Blender 2.80
And I think there is some bug, where .obj file (the exported .obj file)
contains 32 face arguments. That's sus
And I think tinyobj loader didn't expected it
So there is bug in blender and at the same time in tinyobj loader
I know, I use outdated Blender...

So I wrote this script, to remove the remaining 28 arguments
"""

from os.path import exists, join
from os import walk

assert (exists("./Makefile") and \
        exists("./resources/")), Exception("Bad environment")

def short(string, max, space=False):
    return string[:max] + (" ..." if space else "...")

def replace_line(file, index, by):
    with open(file, "r") as file_input:
        file_content = file_input.readlines()
    
    file_content[index] = by + "\n"
    
    with open(file, "w") as file_output:
        file_output.writelines(file_content)

for root, dirs, files in walk("./resources/", topdown=True):
    for file in files:
        file_path = join("./resources", file)
        if file_path.endswith(".obj"):
            print("[FIXOBJ.PY] Fixing " + file_path + "...")
            bad = 0
            with open(file_path, "r") as file_content:
                for line_number, line_raw in enumerate(file_content):
                    line = line_raw.rstrip("\n")
                    if line.startswith("f "):
                        if line.count(" ") > 4:
                            print("[FIXOBJ.PY] Found bad line (line " + str(line_number + 1) + "):")
                            print("    " + short(line, 55))
                            indices = [index for index, char in enumerate(line) if char == " "]
                            replace_line(file_path, line_number, line[:indices[4]])
                            bad += 1
            if bad < 1:
                print("   ...No bad lines")