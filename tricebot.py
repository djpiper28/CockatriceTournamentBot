import requests

class GameMade:
    def __init__(self, success: str, gameID: int):
        self.success = success
        self.gameName = gameName

class TriceBot:    
    def __init__(self, authToken: str, apiURL: str="https://0.0.0.0:8000"):
        self.authToken = authToken
        self.apiURL = apiURL
        
    # verify = false as self signed ssl certificates will cause errors here
    def req(self, urlpostfix: str, data: str):
        return requests.get(self.apiURL + "/" + urlpostfix, timeout=7.0, data=data,  verify=False).text
        
    def checkauthkey(self):
        return self.req("api/checkauthkey", self.authToken) == "1"
    
    def createGame(self, gamename: str, password: str, playercount: int, spectatorsallowed: bool, spectatorsneedpassword: bool, spectatorscanchat: bool, spectatorscanseehands: bool, onlyregistered: bool):
        body = "authtoken=" + self.authToken + "\n"
        body += "gamename=" + gamename + "\n"
        body += "password=" + password + "\n"
        body += "playerCount=" + str(playercount) + "\n"
        
        body += "spectatorsAllowed="
        if spectatorsallowed:
            body += "1"
        else:
            body +="0"
        body += "\n"
            
        body += "spectatorsNeedPassword="
        if spectatorsneedpassword:
            body += "1"
        else:
            body += "0"
        body += "\n"
        
        body += "spectatorsCanChat="
        if spectatorscanchat:
            body += "1"
        else:
            body +="0"
        body += "\n"
        
        body += "spectatorsCanSeeHands="
        if spectatorscanseehands:
            body += "1"
        else:
            body +="0"
        body += "\n"
        
        body += "onlyRegistered="
        if onlyregistered:
            body += "1"
        else:
            body +="0"
            
        try:
            status = self.req("api/creategame/", body)     
        except OSError as exc:
            # logging.error(f"exception while requesting create game with body:\n{body}", exc_info=exc)
            return GameMade(False, -1)

        if (status == "timeout error" or status == "error 404" or status == "invalid auth token"):
            return GameMade(False, -1)
        
        parts = status.split("=")
        if (len(parts) == 2):
            #jank lmao
            try:
                return GameMade(True, int(parts[1]))
            except:
                return GameMade(False, -1)
        return GameMade(False, -1)
