#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <Arduino.h>
#include "StepperController.h"
#include "Config.h"
#include <SPI.h>
#include <SD.h>
#include "Executor.h"

class GcodeParser;

class FileManager {
private:
  int chipSelect;
  GcodeParser* parser;  // pointer to GcodeParser
  MotionPlanner* planner; 
  String gcodeFiles[20];
  int fileCount;
  File root;
  File entry;
  int selectedIndex;
public:
  FileManager(int cs, GcodeParser* p, MotionPlanner* mp)
    : chipSelect(cs), parser(p), planner(mp), fileCount(0) {}

  void initializeSD() {
    Serial.println("Initializing SD card...");
    if (!SD.begin(chipSelect)) {
      Serial.println("SD init failed!");
      return;
    }
    Serial.println("SD card ready.");
  }

  void listGcodeFiles() {
    root = SD.open("/");

    int index = 1;
    Serial.println("GCODE files on SD card:");

    while (true) {
      entry = root.openNextFile();
      if (!entry) break;

      String nameGCO = entry.name();
      if (nameGCO.endsWith(".gco") || nameGCO.endsWith(".GCO")) {
        Serial.print(index);
        Serial.print(": ");
        Serial.println(nameGCO);
        gcodeFiles[fileCount] = nameGCO;
        fileCount++;
        index++;
      }
      entry.close();
    }
    Serial.println("END LIST");
  }

  String selectFile() {
    Serial.println("Type the number of the file you want to select:");
    while (Serial.available() == 0) {
      // wait for user input
    }

    String input = Serial.readStringUntil('\n');
    selectedIndex = input.toInt() - 1;

    if (selectedIndex < 0 || selectedIndex >= fileCount) {
      Serial.println("Invalid selection.");
      return;
    }

    Serial.print("You selected file: ");
    Serial.println(gcodeFiles[selectedIndex]);
    baseGCO = gcodeFiles[selectedIndex];
    baseGCO.remove(baseGCO.lastIndexOf('.'));  // strip extension

    checkMatchingTxtFile();
  }

  void checkMatchingTxtFile() {
    // Check for matching TXT file
    Serial.println("Searching for identical TXT file...");
    fileNameIdentical = false;  // reset flag
    root = SD.open("/");
    while (true) {
      entry = root.openNextFile();
      if (!entry) break;

      String nameTXT = entry.name();
      if (nameTXT.endsWith(".TXT") || nameTXT.endsWith(".txt")) {
        String baseTXT = nameTXT;
        baseTXT.remove(baseTXT.lastIndexOf('.'));
        if (baseTXT == baseGCO) {
          fileNameIdentical = true;
        }
      }
      entry.close();
    }

    String txtFileName = baseGCO + ".TXT";

    if (fileNameIdentical) {
      Serial.println("Identical TXT file found, checking IDs...");

      File source = SD.open(gcodeFiles[selectedIndex]);
      File target = SD.open(txtFileName);

      if (source && target) {
        String sourceID = readFirstLine(source);
        String targetID = readFirstLine(target);
        sourceID.trim();
        targetID.trim();
        source.close();
        target.close();

        Serial.print("Source ID: ");
        Serial.println(sourceID);
        Serial.print("Target ID: ");
        Serial.println(targetID);

        if (sourceID == targetID) {
          Serial.println("IDs match -> Executing TXT file...");
        } else {
          Serial.println("IDs do not match -> Overwriting TXT file...");
          SD.remove(txtFileName);
          File newTarget = SD.open(txtFileName, FILE_WRITE);
          if (newTarget) {
            newTarget.println(sourceID);
            newTarget.close();
            parser->processGCODE((char*)gcodeFiles[selectedIndex].c_str(), (char*)txtFileName.c_str());
          }
        }
      } else {
        Serial.println("Error opening files for comparison.");
      }

    } else {
      Serial.println("No identical TXT file found -> Creating new file...");
      File newTarget = SD.open(txtFileName, FILE_WRITE);
      if (newTarget) {
        File source = SD.open(gcodeFiles[selectedIndex]);
        if (source) {
          String sourceID = readFirstLine(source);
          source.close();
          newTarget.println(sourceID);
        }
        newTarget.close();
        parser->processGCODE((char*)gcodeFiles[selectedIndex].c_str(), (char*)txtFileName.c_str());
      }
    }

    File toExecute = SD.open(txtFileName, FILE_READ);
    if (toExecute) {
      Executor executor(*planner);
      executor.excecuteTargetFile(toExecute);
    } else {
      Serial.println("Error: could not reopen TXT for execution");
    }
  }

  String readFirstLine(File file) {
    String line = "";
    while (file.available()) {
      char c = file.read();
      if (c == '\n') break;  // stop at first newline
      line += c;
    }
    return line;
  }
};

#endif