import requests
from requests.auth import HTTPBasicAuth
from requests.packages.urllib3.exceptions import InsecureRequestWarning
import subprocess
import json
import threading
import time

class LCU:
    def __init__(self):
        output = subprocess.Popen(["main.exe"], stdout=subprocess.PIPE).communicate()[0]
        if "Process not running" in output:
            print "Process not running"
            self.connected = False
            return
        self.token = output.split("token ")[1].split('\n')[0].rstrip()
        self.port = output.split("port ")[1].split('\n')[0].rstrip()
        self.connected = True
    
    def get(self, path):
        baseUrl = "https://127.0.0.1:"+self.port+"/"
        res = requests.get(baseUrl+path, auth=HTTPBasicAuth('riot',self.token), verify=False)
        return res


class Summoner:
    def __init__(self, name=False, id=-1):
        # if name is provided
        self.lcu = LCU()
        if not self.lcu.connected:
            self.success = False
            return
        
        if name:
            self.name = name
            res = self.lcu.get("lol-summoner/v1/summoners?name="+self.name.decode('gb2312'))
            if res.status_code != 200:
                print "Invalid summoner name"
                print res.json()['message']
                self.success = False
                return None
            else:
                self.summoner = res.json()
                self.success = True

            return
        # if summoner id is provided
        elif id != -1:
            self.id = id
            res = self.lcu.get("lol-summoner/v1/summoners/"+str(self.id))
            
            if res.status_code != 200:
                print "Invalid summoner id,", res.json()['message']
                self.success = False
                return
            else:
                self.summoner = res.json()
                self.success = True
            return

    def getRankStats(self, queueType):
        self.rankStats = self.lcu.get("lol-ranked/v1/ranked-stats/"+self.summoner['puuid']).json()
        options = ["RANKED_SOLO_5x5", "RANKED_FLEX_SR", "RANKED_FLEX_TT", "RANKED_TFT"]
        if queueType in options:
            return self.rankStats["queueMap"][queueType]["tier"]+" "+self.rankStats["queueMap"][queueType]["division"]
        else:
            print "Please input a valid queue type"
        return

def printMenu():
    print """
    Hello Summoner, what would you like to know today?
    1. Get rank of a single summoner
    2. Get rank of my team
    """

def printRankSummoner():
    options = ["RANKED_SOLO_5x5", "RANKED_FLEX_SR", "RANKED_FLEX_TT", "RANKED_TFT"]
    name = raw_input("Enter Summoner Name:")
    print """
    Choose a queue map type
    1. RANKED_SOLO_5x5
    2. RANKED_FLEX_SR
    3. RANKED_FLEX_TT
    4. RANKED_TFT
    """
    choice = raw_input(">")
    if choice.isdigit():
        queueMapIndex = int(choice)-1
    else:
        queueMapIndex = 0
    
    if queueMapIndex < 4:
        queueMap = options[queueMapIndex]
    else:
        queueMap = options[0]
    
    target = Summoner(name=name)
    if not target.success:
        return
    else:
        print target.getRankStats(queueMap)

def printRankTeam():
    lcu = LCU()
    if not lcu.connected:
        return
    
    res = lcu.get("lol-champ-select/v1/session")
    if res.status_code != 200:
        print "Are you sure you are in champ select yet?"
        return

    res = res.json()
    for member in res["myTeam"]:
        s = Summoner(id=member['summonerId'])
        if s.success:
            print s.summoner['displayName'], s.getRankStats('RANKED_SOLO_5x5')
    
    return


def main():
    # Looping menu
    while True:
        printMenu()
        choice = raw_input(">")
        if choice == 'q':
            break
        elif choice == '1':
            printRankSummoner()
        elif choice == '2':
            printRankTeam()

requests.packages.urllib3.disable_warnings(InsecureRequestWarning)
if __name__ == "__main__":
    main()