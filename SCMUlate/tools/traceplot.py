#!/usr/bin/env python3

import plotly.graph_objects as go
from plotly.subplots import make_subplots
from enum import Enum
import argparse
import json
import sys
from collections import OrderedDict
import numpy as np

def EngNumber(v,precision):
    return str(v)
try:
    from engineering_notation import EngNumber
except ImportError as e:
    print("No engineering_notation package. Numbers will be shown long and ugly")
    pass  

''' List of possible colors
aliceblue, antiquewhite, aqua, aquamarine, azure,
beige, bisque, black, blanchedalmond, blue,
blueviolet, brown, burlywood, cadetblue,
chartreuse, chocolate, coral, cornflowerblue,
cornsilk, crimson, cyan, darkblue, darkcyan,
darkgoldenrod, darkgray, darkgrey, darkgreen,
darkkhaki, darkmagenta, darkolivegreen, darkorange,
darkorchid, darkred, darksalmon, darkseagreen,
darkslateblue, darkslategray, darkslategrey,
darkturquoise, darkviolet, deeppink, deepskyblue,
dimgray, dimgrey, dodgerblue, firebrick,
floralwhite, forestgreen, fuchsia, gainsboro,
ghostwhite, gold, goldenrod, gray, grey, green,
greenyellow, honeydew, hotpink, indianred, indigo,
ivory, khaki, lavender, lavenderblush, lawngreen,
lemonchiffon, lightblue, lightcoral, lightcyan,
lightgoldenrodyellow, lightgray, lightgrey,
lightgreen, lightpink, lightsalmon, lightseagreen,
lightskyblue, lightslategray, lightslategrey,
lightsteelblue, lightyellow, lime, limegreen,
linen, magenta, maroon, mediumaquamarine,
mediumblue, mediumorchid, mediumpurple,
mediumseagreen, mediumslateblue, mediumspringgreen,
mediumturquoise, mediumvioletred, midnightblue,
mintcream, mistyrose, moccasin, navajowhite, navy,
oldlace, olive, olivedrab, orange, orangered,
orchid, palegoldenrod, palegreen, paleturquoise,
palevioletred, papayawhip, peachpuff, peru, pink,
plum, powderblue, purple, red, rosybrown,
royalblue, saddlebrown, salmon, sandybrown,
seagreen, seashell, sienna, silver, skyblue,
slateblue, slategray, slategrey, snow, springgreen,
steelblue, tan, teal, thistle, tomato, turquoise,
violet, wheat, white, whitesmoke, yellow,
yellowgreen
'''

color_map = {
    "SYS_TIMER": "lightslategray",
    "SU_TIMER": "peachpuff",
    "MEM_TIMER": "midnightblue",
    "CUMEM_TIMER": "mediumseagreen",
    "SYS_START": "lightgray",
    "SYS_END": "yellow",
    "SU_START": "thistle",
    "SU_END": "firebrick",
    "FETCH_DECODE_INSTRUCTION": "mediumblue",
    "DISPATCH_INSTRUCTION": "blue",
    "EXECUTE_CONTROL_INSTRUCTION": "darkblue",
    "EXECUTE_ARITH_INSTRUCTION": "black",
    "CUMEM_IDLE": "whitesmoke",
    "CUMEM_START": "mediumaquamarine",
    "CUMEM_END": "linen",
    "CUMEM_EXECUTION_MEM": "yellowgreen",
    "CUMEM_EXECUTION_COD": "darkviolet",
    "CUMEM_IDLE": "darkslateblue",
    "SCM_MACHINE": "mediumseagreen"
}

class counter_type(Enum):
    SYS_TIMER = 0
    SU_TIMER = 1
    MEM_TIMER = 2
    CUMEM_TIMER = 3

class SYS_event(Enum):
    SYS_START = 0
    SYS_END = 1

class SU_event(Enum):
    SU_START = 0
    SU_END = 1
    FETCH_DECODE_INSTRUCTION = 2
    DISPATCH_INSTRUCTION = 3
    EXECUTE_CONTROL_INSTRUCTION = 4
    EXECUTE_ARITH_INSTRUCTION = 5
    SU_IDLE = 6

class CUMEM_event(Enum):
    CUMEM_START = 0
    CUMEM_END = 1
    CUMEM_EXECUTION_COD = 2
    CUMEM_EXECUTION_MEM = 3
    CUMEM_IDLE = 4

def getEnumPerType(typeName, eventID):
    if typeName == "SYS_TIMER":
        return SYS_event(eventID)
    if typeName == "SU_TIMER":
        return SU_event(eventID)
    if typeName == "CUMEM_TIMER":
        return CUMEM_event(eventID)
    return None


def load_file(fileName):
    try:
        with open(fileName) as f:
            return json.load(f)
    except:
        print("Error opening file", file=sys.stderr)
        exit()

class printStats:
    map_of_instructions = {}
    def addNewStat(self, name, start, end, eventType, description):
        if description.split(" ")[0] not in self.map_of_instructions:
            self.map_of_instructions[description.split(" ")[0]] = []
        self.map_of_instructions[description.split(" ")[0]].append(end)
    def print(self):
        print(f" Instruction\tExecutionTimes\taverageTime")
        for inst, list_exec_times in self.map_of_instructions.items():
            print(f"{inst}\t{len(list_exec_times)}\t{np.average(list_exec_times):0.2e}")
class traces:
    numPlots = 0
    plotNum = list()
    plotOrder = list()
    y = {}
    x = {}
    utilization = {}
    memory = {}
    compute = {}
    legend = {}
    bases = {}
    colors = {}
    minPlot = sys.maxsize
    maxPlot = 0

    fig = None
    def __init__(self):
        self.numPlots = 0
        self.plotNum = list()
        self.plotOrder = list()
        self.curNumPlots = 0
        self.y = {}
        self.x = {}
        self.utilization = {}
        self.memory = {}
        self.compute = {}
        self.legend = {}
        self.bases = {}
        self.colors = {}
        self.minPlot = sys.maxsize
        self.maxPlot = 0
        self.fig = None
        self.config = dict({'scrollZoom': True})

    
    def addNewTrace(self, name):
        if name in self.plotNum:
            print("Trying to add same plot twice", file=sys.stderr)
            exit()
        self.plotNum.append(name)
        self.numPlots += 1

    def addNewPlot(self, name, start, end, eventType, description):
        if name not in self.y:
            self.y[name] = []
        if name not in self.x:
            self.x[name] = []
        if name not in self.utilization:
            self.utilization[name] = 0
        if name not in self.memory:
            self.memory[name] = 0
        if name not in self.compute:
            self.compute[name] = 0
        if name not in self.bases:
            self.bases[name] = []
        if name not in self.legend:
            self.legend[name] = []
        if name not in self.colors:
            self.colors[name] = []
        if name not in self.plotNum:
            print("Plot does not exist", file=sys.stderr)
            exit()
        if (start < self.minPlot):
            self.minPlot = start
        if (start + end > self.maxPlot):
            self.maxPlot = start + end
        if (eventType == "CUMEM_EXECUTION_MEM"):
            self.memory[name] += end
        if (eventType == "CUMEM_EXECUTION_COD"):
            self.compute[name] += end
        self.y[name].append(name)
        self.x[name].append(end)
        self.utilization[name] += end
        self.bases[name].append(start)
        self.legend[name].append(f'{eventType} [{EngNumber(str(end),precision=3)}s]<br>{description}')
        self.colors[name].append(color_map[eventType])
    
    def plotTrace(self, subText = ""):
        self.fig = go.Figure()
        title = f'SCMULate trace: min = {EngNumber(self.minPlot)}s, max = {EngNumber(self.maxPlot)}s, total = {EngNumber(self.maxPlot-self.minPlot)}s <br>{subText} '
        secondLabel = ""
        print(title)
        for name in sorted(self.plotNum, reverse=True):
            if name in self.x:
                util = self.utilization[name]/(self.maxPlot - self.minPlot)*100
                self.fig.add_trace(
                    go.Bar( x = np.array(self.x[name])*1000, 
                            y = self.y[name], 
                            base = np.array(self.bases[name])*1000, 
                            hovertext = self.legend[name], 
                            orientation = 'h', 
                            marker_color = self.colors[name],
                            name = f'{name} [{util:.2f}% util]'))
                memUtil = self.memory[name]/self.utilization[name]*100
                compUtil = self.compute[name]/self.utilization[name]*100

                secondLabel += f'{name} [mem = {memUtil:.2f}%, comp = {compUtil:.2f}%]<br>'
            else:
                if name == "SCM_MACHINE":
                    self.fig.add_trace(
                        go.Bar( x = [(self.maxPlot - self.minPlot)*1000], 
                                y = [name],
                                base = [self.minPlot*1000], 
                                orientation = 'h',
                                hovertext = f"Duration = {EngNumber(self.maxPlot - self.minPlot)}s",
                                marker_color = color_map[name],
                                name = f'{name}'))
                else:
                    self.fig.add_trace(
                        go.Bar( x = [0], 
                                y = [name],
                                base = [0],
                                orientation = 'h',
                                name = name))
        self.fig.update_layout(
            barmode = 'relative', 
            title_text = title,
            width=1600, height=800,
            dragmode = False, 
            yaxis = {'fixedrange': True,
                     'title': "SCM Unit", 
                     'tickson': 'boundaries'},
            font = dict(size=30),
            xaxis = {
                'categoryorder': 'array',
                'title': "Time (ms)"},
            annotations=[
                go.layout.Annotation(
                    text=secondLabel,
                    align='left',
                    showarrow=False,
                    xref='paper',
                    yref='paper',
                    xanchor='left',
                    x=1.001,
                    y=0,
                    bordercolor='black',
                    borderwidth=1, 
                    font=dict(size=18)
                )
            ])
        self.fig.show(config=self.config)
        




def main():
    parser = argparse.ArgumentParser(description='Plots the trace result of SCMUlate')
    parser.add_argument('--input', '-i', dest='fileName', action='store', nargs=1,
                    help='File name to be plot', required=True)
    parser.add_argument('--stats-only', '-so', dest='statsOnly', action='store_true', help='Print stats only')
    parser.add_argument('--subtitle', '-st', dest='subtitle', action='store', nargs=1, help="Subtitle to be added to the plot", required=False)

    args = parser.parse_args()
    fileName = args.fileName[0]
    statsOnly = args.statsOnly
    subText = ""
    if (args.subtitle != None):
        subText = args.subtitle[0]
    tracePloter = traces()
    stats = printStats()
    if fileName != "":
        data = load_file(fileName)
        for name, counter in data.items():
            tracePloter.addNewTrace(name)
            prevTime = None
            prevType = ""
            counter_t = counter_type(int(counter["counter type"])).name
            for event in counter["events"]:
                if prevTime is None:
                    prevTime = event["value"]
                    prevType = event["type"]
                    prevEvent = event["description"]
                else:
                    typeEnum = getEnumPerType(counter_t, int(prevType))
                    if (typeEnum.name[-4:] != "IDLE" and typeEnum.name[-3:] != "END" and typeEnum.name[-5:] != "START"):
                        description = prevEvent
                        if "hw_counters" in event:
                            for counter, value in event["hw_counters"]:
                                description += "<br>" + counter + ": " + str(value)
                        if (event["value"] - prevTime != 0):
                            tracePloter.addNewPlot(name, prevTime, event["value"] - prevTime, typeEnum.name, description)
                            stats.addNewStat(name, prevTime, event["value"] - prevTime, typeEnum.name, description)
                    prevTime = event["value"]
                    prevType = event["type"]
                    prevEvent = event["description"]
        if (not statsOnly):
            tracePloter.plotTrace(subText)
        else:
            stats.print()
    else:
        print("No file specified")
        parser.print_help()
        exit()

def jupyter(fileName = "",subText = "",):
    tracePloter = traces()
    if fileName != "":
        data = load_file(fileName)
        for name, counter in data.items():
            tracePloter.addNewTrace(name)
            prevTime = None
            prevType = ""
            counter_t = counter_type(int(counter["counter type"])).name
            for event in counter["events"]:
                if prevTime is None:
                    prevTime = event["value"]
                    prevType = event["type"]
                    prevEvent = event["description"]
                else:
                    typeEnum = getEnumPerType(counter_t, int(prevType))
                    if (typeEnum.name[-4:] != "IDLE" and typeEnum.name[-3:] != "END" and typeEnum.name[-5:] != "START"):
                        description = prevEvent
                        if "hw_counters" in event:
                            for counter in event["hw_counters"]:
                                description += "<br>" + counter + ": " + str(event["hw_counters"][counter])

                        tracePloter.addNewPlot(name, prevTime, event["value"] - prevTime, typeEnum.name, description )
                    prevTime = event["value"]
                    prevType = event["type"]
                    prevEvent = event["description"]

        tracePloter.plotTrace(subText)
    else:
        print("No file specified")

if __name__ == "__main__":
    main()
