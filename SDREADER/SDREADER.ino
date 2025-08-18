#include <SPI.h>
#include <SD.h>

const int chipSelect = 53;

String gcodeFiles[20];
int fileCount = 0;
bool fileNameIdentical = false;
String baseGCO;  // keep selected base filename

// Function to read the first line of a file
String readFirstLine(File file) {
  String line = "";
  while (file.available()) {
    char c = file.read();
    if (c == '\n') break;  // stop at first newline
    line += c;
  }
  return line;
}

void setup() {
  Serial.begin(9600);
  pinMode(38, INPUT_PULLUP);

  Serial.println("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("SD init failed!");
    return;
  }
  Serial.println("SD card ready.");

  File root = SD.open("/");
  File entry;

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

  Serial.println("Type the number of the file you want to select:");

  while (Serial.available() == 0) {
    // just wait for user input
  }

  String input = Serial.readStringUntil('\n');
  int selectedIndex = input.toInt() - 1;

  if (selectedIndex >= 0 && selectedIndex < fileCount) {
    Serial.print("You selected file: ");
    Serial.println(gcodeFiles[selectedIndex]);
    baseGCO = gcodeFiles[selectedIndex];
    baseGCO.remove(baseGCO.lastIndexOf('.'));  // strip extension
  } else {
    Serial.println("Invalid selection.");
    return;
  }

  // Check for matching TXT file
  Serial.println("Searching for identical TXT file...");
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

    // Open GCO source and TXT target
    File source = SD.open(gcodeFiles[selectedIndex]);
    File target = SD.open(txtFileName);

    if (source && target) {
      String sourceID = readFirstLine(source);
      String targetID = readFirstLine(target);
      source.close();
      target.close();

      Serial.print("Source ID: ");
      Serial.println(sourceID);
      Serial.print("Target ID: ");
      Serial.println(targetID);

      if (sourceID == targetID) {
        Serial.println("IDs match -> Executing TXT file...");
        // Place "execution" logic here
      } else {
        Serial.println("IDs do not match -> Overwriting TXT file...");
        SD.remove(txtFileName);  // delete old file
        File newTarget = SD.open(txtFileName, FILE_WRITE);
        if (newTarget) {
          newTarget.println(sourceID);  // write new ID
          // Add additional data as needed
          newTarget.close();
        }
      }
    } else {
      Serial.println("Error opening files for comparison.");
    }

  } else {
    Serial.println("No identical TXT file found -> Creating new file...");
    File newTarget = SD.open(txtFileName, FILE_WRITE);
    if (newTarget) {
      // For simplicity, use GCO's ID as first line
      File source = SD.open(gcodeFiles[selectedIndex]);
      if (source) {
        String sourceID = readFirstLine(source);
        source.close();
        newTarget.println(sourceID);
      }
      // Add additional data as needed
      newTarget.close();
    }
  }
}

void loop() {
  // Nothing here yet
}
