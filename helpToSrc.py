#!/usr/bin/env python3

def genCssFile(outputName, defineName, inputName):
    import os
    import re

    thisDir = os.path.dirname(os.path.abspath(__file__))
    sourcePath = os.path.join(thisDir, "src", inputName)

    cssStr = ""
    genSource = []
    with open(sourcePath) as fp:
        for line in fp:
            line = line[:-1] # remove newline
            genSource.append(line.replace('"', '\\"'))
            cssStr += line

    genSourceString = "\\n\\\n".join(genSource)
    genSourceContents = f"""#ifndef {defineName}
    #define {defineName} "{genSourceString}"
    #endif
    """
    with open(outputName, "w+") as fp:
        fp.write(genSourceContents)
    return cssStr

css = genCssFile("page_css.h", "PAGE_CSS", "apistyle.css")

def genFile(outputName, defineName, inputName):
    import os
    import re

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
            if line == "{CSS}":
                line = css
            
            if line == "{INDEX}":
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
                    f'<{indexTag}><a href=\\"#{escaped}\\">{name}</a></{indexTag}>'
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

genFile("help_page.h", "API_HELP", "apihelp.html")
genFile("faq_page.h", "FAQ", "apifaq.html")
genFile("index.h", "INDEX", "index.html")
