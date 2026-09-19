from flask import Flask, request
from tabulate import tabulate
import time
from colorama import init, Style
from geopy.distance import geodesic
import csv
from tkinter import filedialog, Tk
import os

localRefPH = 0
localRefEC = 0
dataIndex = 0

refDataTURB = 150 #put in real numbers!!!!!!!!! put in threshold value
refDataDOM = 150
stuckTimes = 0

stuckDistanceThreshold = 1 #meters
stuckTimeThreshold = 5 #times

voltageMaxADC = 1750 #mv
thresholdPH = 0.5 #PH steps
thresholdEC = 20 #put in real value
PH7 = 100

timeOut = 10
masterData = []
highLightTURB = []
highLightDOM = []
highLightPH = []
highLightEC = []

allSources = []

app = Flask(__name__)

init(autoreset = True)

root = Tk()
root.withdraw() 

def findPollutionPoints(): #highLight(TYPE) = [TYPE, LAT, LON, TURB, score, index]
    for i in range(len(masterData)): 
        TURB = int(masterData[i][2])
        if TURB <= refDataTURB: #if it is polluted !!!!!!!!!!!!!!!!!!!!!!!!!!!check sign!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            print("TURB")
            LAT = masterData[i][0]
            LON = masterData[i][1]
            scoreTURB = TURB / refDataTURB #lower better
            highLightTURB.append(["TURB", LAT, LON, TURB, scoreTURB, i])
            print("Found pollution point")

        DOM = int(masterData[i][3])
        if DOM <= refDataDOM:
            print("DOM")
            LAT = masterData[i][0]
            LON = masterData[i][1]
            scoreDOM = DOM / refDataDOM
            scoreDOM = DOM / refDataDOM #lower better
            highLightDOM.append(["DOM", LAT, LON, DOM, scoreDOM, i])
            print("Pollution pollution point")

        PH = int(masterData[i][4])
        if (((abs(PH - int(localRefPH)) * voltageMaxADC) / 4095) - PH7) / -60 > thresholdPH:
            print("PH")
            LAT = masterData[i][0]
            LON = masterData[i][1]
            scorePH = ((PH - localRefPH) / (4095/voltageMaxADC)) #closer to 0 better
            highLightPH.append(["PH", LAT, LON, PH, scorePH, i])
            print("Found pollution point")

        EC = int(masterData[i][5])
        if EC - int(localRefEC) > thresholdEC:
            print("EC")
            LAT = masterData[i][0]
            LON = masterData[i][1]
            scoreEC = EC - localRefEC #closer to 0 better
            highLightEC.append(["EC", LAT, LON, EC, scoreEC, i])
            print("Found pollution point")

def findSources(pollutionArray): #pollutionArray = highLight(TYPE) sources
    for i in range(len(pollutionArray)):
        if i == 0:
            allSources.append(pollutionArray[i])
        elif iPrev + 1 != pollutionArray[i][-1]: #if the last index is not right before the current index
            allSources.append(pollutionArray[i])
        iPrev = pollutionArray[i][-1]

def groupedSources(): #[LAT, LON, TYPE1, TYPE2*, TYPE3*, TYPE4*]
    sourcesDict = {}

    for i in range (len(allSources)):
        index = allSources[i][-1]
        TYPE = allSources[i][0]
        if index in sourcesDict and TYPE not in sourcesDict[index]:
                sourcesDict[index].append(TYPE)
        elif index - 1 in sourcesDict and TYPE not in sourcesDict[index - 1]:
                sourcesDict[index - 1].append(TYPE)
        elif index - 2 in sourcesDict and TYPE not in sourcesDict[index - 2]:
                sourcesDict[index - 2].append(TYPE)
        elif index - 3 in sourcesDict and TYPE not in sourcesDict[index - 3]:
                sourcesDict[index - 3].append(TYPE)
        else:
            LAT = allSources[i][1]
            LON = allSources[i][2]
            sourcesDict[index] = [LAT, LON, TYPE]

    sourcesArray = []
    for key in sourcesDict.keys():
        sourcesArray.append(sourcesDict[key])
    return sourcesArray

def printAnalysis():
    findPollutionPoints()
    findSources(highLightTURB)
    findSources(highLightDOM)
    findSources(highLightPH)
    findSources(highLightEC)
    avgTURB = 0
    avgDOM = 0
    avgPH = 0
    avgEC = 0

    headers = ["LAT", "LON", "TYPE 1", "TYPE 2", "TYPE 3", "TYPE 4"]
    print(groupedSources())
    print(tabulate(groupedSources(), headers, tablefmt = "grid"))
    for i in range(len(masterData)):
        avgTURB = (avgTURB + masterData[i][2]) / (i + 1) #LAT, LON, TURB, DOM, PH, EC, dataIndex
        avgDOM = (avgDOM + masterData[i][3]) / (i + 1)
        avgPH = (avgPH + masterData[i][4]) / (i + 1)
        avgEC = (avgEC + masterData[i][5]) / (i + 1)
        
    print()
    print("Average:")
    print("TURB: " + str(avgTURB))
    print("DOM: " + str(avgDOM))
    print("PH: " + str(avgPH))
    print("EC: " + str(avgEC))

    #save csv file
    if "Y" in input("Download CSV file? (Y/N): "):
        filePath = filedialog.asksaveasfilename (
        defaultextension=".csv",
        filetypes=[("CSV files", "*.csv"), ("All files", "*.*")],
        title="Save CSV File"
        )
        with open(filePath, "w", newline = "", encoding = "utf-8") as file:
            groupedSourcesCSV = headers + groupedSources
            writer = csv.writer(file)
            writer.writerows(groupedSourcesCSV)
            print("CSV downloaded")
    else:
        print("CSV cancled")

    highLightTURB.clear()
    highLightDOM.clear()
    highLightPH.clear()
    highLightEC.clear()
    allSources.clear()

@app.route('/', methods=['POST', 'GET'])
def read():
    global localRefPH, localRefEC, dataIndex

    data = request.get_data().decode()
    print("Data recieved:")
    print(data)

    if "OK?" in data:
        print("Buoy: OK? OK")
        return "OK", 200
    
    elif "Local water stats" in data:
        localRefPH = int(data[data.find("PH:") + 3 : data.find(",")])
        localRefEC = int(data[data.find("EC:") + 3 : data.find(",", data.find("EC:"))])
        print("PH: " + str(localRefPH) + ", EC: " + str(localRefEC))

    elif "Measurement Delay?" in data:
        userInput = input("Measurement delay (1 = 10s, 2 = 20s, 3 = 30s, etc, up to 9): ")
        print("user input: " + userInput)
        return ":" + userInput + ","
    
    elif "TURB" in data and "DOM" in data and "PH" in data and "EC" in data and "LAT" in data and "LON" in data:
        TURB = data[data.index("TURB:") + 5 : data.index(",", data.index("TURB:"))]
        DOM = data[data.index("DOM:") + 4 : data.index(",", data.index("DOM:"))]
        PH = data[data.index("PH:") + 3 : data.index(",", data.index("PH:"))]
        EC = data[data.index("EC:") + 3 : data.index(",", data.index("EC:"))]
        LAT = data[data.index("LAT:") + 4 : data.index(",", data.index("LAT:"))]
        LON = data[data.index("LON:") + 4 : data.index(",", data.index("LON:"))]
        print("Data recieved")
        print(TURB, DOM, PH, EC, LAT, LON, sep = "\n")

        if "N" in LAT:
            secondsLAT = float(LAT[2 : LAT.index("N")])
            degreesLAT = float(LAT[ : 2])
        elif "S" in LAT:
            secondsLAT = -float(LAT[2 : LAT.index("S")])
            degreesLAT = -float(LAT[ : 2])
        else:
            print("Coordinates error")
            while(1):
                time.sleep(1)

        LAT = degreesLAT + (secondsLAT / 60)
        print("Decimal LAT: " + str(LAT))
        
        if "E" in LON:
            secondsLON = float(LON[2 : LON.index("E")])
            degreesLON = float(LON[ : 2])
        elif "W" in LON:
            secondsLON = -float(LON[2 : LON.index("W")])
            degreesLON = -float(LON[ : 2])
        else:
            print("Coordinates error")
            while(1):
                time.sleep(1)

        LON = degreesLON + (secondsLON / 60)
        print("Decimal LON: " + str(LON))
        
        masterData.append([LAT, LON, TURB, DOM, PH, EC, dataIndex])

    elif "Done" in data:
        print("Task done, buoy has finished data collection.")
        userInput = input("Do you want to print analysis? (Y/N): ")
        while True: 
            if userInput in ("y", "Y"):
                printAnalysis()
                break
            elif userInput in ("n", "N"):
                print("Analysis skipped")
                break
            else:
                userInput = input("Do you want to print analysis? (Y/N): ")

    else:
        print("ERROR: Transmittion incomplete")
        return "Transmittion incomplete"

    dataIndex += 1
    return "OK", 200


try:
    app.run(debug=True, host='0.0.0.0', port=5000)
except KeyboardInterrupt:
    printAnalysis()