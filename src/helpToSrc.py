helpFile = open("helppage.html", "r") 
anchorPoints = []
tmpSource = []

for line in helpFile:
    temp = line.split("name=\"")
    if (len(temp) > 1):
        anchorPoints.append(temp[1].split("\"")[0])
    
        temp2 = temp[1].split("\"")
        line = temp[0] + "name=\"" + temp2[0].replace(" ", "%20").replace("/", "%2F").replace("%", "%25") + "\"" + temp2[1]
        
        
    tmpSource.append(line)
    
helpFile.close()

anchorPointsProcessed = ""

for anchorPoint in anchorPoints:
    anchorPointsProcessed += "<p><a href=\\\"#" + anchorPoint.replace(" ", "%20").replace("/", "%2F").replace("%", "%25") + "\\\">"+anchorPoint+"</a></p>\\n"
        
gen = ""
genSource = "#ifndef HELP_STR\n#define HELP_STR \""
for line in tmpSource:
    if (line == "{INDEX}\n"):
        genSource += anchorPointsProcessed
        gen+=anchorPointsProcessed
    else:
        line = line.replace("\"", "\\\"").replace("\n", "\\n")
        #print(line)
        gen+=line
        genSource += line

genfile = open("helppage.gen.html", "w+")
genfile.write(gen.replace("\\n", "\n").replace("\\\"", "\""))
genfile.close()

outputFile = open("helppage.h", "w+")
outputFile.write(genSource+"\"\n#endif\n")
outputFile.close()
