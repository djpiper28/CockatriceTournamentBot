#!/usr/bin/env python3
import os
import re

thisDir = os.path.dirname(os.path.abspath(__file__))
sourcePath = os.path.join(thisDir, "src", "helppage.html")
outputName = "helppage.h"
headerNameRegex = re.compile(r'(.*<.*name=")(.*)(">.*)')


def escapesString(value):
    return value.replace(" ", "%20").replace("/", "%2F").replace("%", "%25")


anchorPointsProcessed = []
genSource = []
with open(sourcePath) as fp:
    for line in fp:
        line = line[:-1] # remove newline
        if line == "{INDEX}":
            indexLocation = len(genSource)
            genSource.append("")
            continue

        match = headerNameRegex.fullmatch(line)
        if match:
            first, name, rest = match.groups()
            escaped = escapesString(name)
            anchorPointsProcessed.append(
                f'<p><a href=\\"#{escaped}\\">{name}</a></p>'
            )
            line = "".join((first, escaped, rest))

        genSource.append(line.replace('"', '\\"'))

try:
    indexLocation
except NameError:
    raise RuntimeError(
        "html source file does not contain index marker"
    ) from None

anchorPointsString = "\\n\\\n".join(anchorPointsProcessed)
genSource[indexLocation] = anchorPointsString
genSourceString = "\\n\\\n".join(genSource)
genSourceContents = f"""#ifndef HELP_STR
#define HELP_STR "{genSourceString}"
#endif
"""
with open(outputName, "w+") as fp:
    fp.write(genSourceContents)
