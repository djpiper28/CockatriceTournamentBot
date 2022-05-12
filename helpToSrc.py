#!/usr/bin/env python3

import os
import re

# For pretty printing
index_ident_level = 5
ident_string = "    "

def genCssFile(outputName, defineName, inputName):
    thisDir = os.path.dirname(os.path.abspath(__file__))
    sourcePath = os.path.join(thisDir, "src", inputName)

    cssStr = ""
    genSource = ["""
        <link rel="shortcut icon" type="image/x-icon" href="/favicon.ico" />
        <link rel="icon" type="image/x-icon" href="/favicon.ico" />
        <style type="text/css">
        """.replace('"', '\\"').replace("\n", "")]
    with open(sourcePath) as fp:
        for line in fp:
            line = line[:-1] # remove newline
            genSource.append(line.replace('"', '\\"'))
            cssStr += line

    genSource.append("</style>")
    genSourceString = "\\n".join(genSource).replace("\n", "")
    genSourceContents = f"""#ifndef {defineName}
    #define {defineName} "{genSourceString}"
    #endif
    """
    with open(outputName, "w+") as fp:
        fp.write(genSourceContents)
    return cssStr

css = genCssFile("page_css.h", "PAGE_CSS", "www/api_style.css")

def genFile(outputName, defineName, inputName):
    thisDir = os.path.dirname(os.path.abspath(__file__))
    sourcePath = os.path.join(thisDir, "src", inputName)
    headerNameRegex = re.compile(r'(.*<)([a-zA-Z0-9]+)(.*id=")(.*)(">.*)')


    def escapesString(value):
        return value.replace(" ", "%20").replace("/", "%2F").replace("%", "%25")


    anchorPointsProcessed = []
    genSource = []
    with open(sourcePath) as fp:
        for line in fp:
            line = line[:-1] # remove newline

            # Using in to ignore indenting.
            # And comments, so they don't get picked up
            # in the documentation randomly.
            if "<!-- {CSS} -->" in line:
                line = css

            if "<!-- {INDEX} -->" in line: 
                indexLocation = len(genSource)
                genSource.append("")
                continue

            match = headerNameRegex.fullmatch(line)
            if match:
                first, tag, prop, name, rest = match.groups()
                indexTag = "p"
                if tag == "h1" or tag == "h2" or tag == "h3":
                    indexTag = tag
                    
                escaped = escapesString(name)
                anchorPointsProcessed.append(
                    (index_ident_level * ident_string) + f'<{indexTag}><a href=\\"#{escaped}\\">{name}</a></{indexTag}>'
                )
                #line = "".join((first, escaped, rest))

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
    genSourceContents = f"""#ifndef {defineName}
    #define {defineName} "{genSourceString}"
    #endif
    """
    with open(outputName, "w+") as fp:
        fp.write(genSourceContents)

genFile("help_page.h", "API_HELP", "www/api_help.html")
genFile("faq_page.h", "FAQ", "www/api_faq.html")
genFile("index.h", "INDEX", "www/index.html")
