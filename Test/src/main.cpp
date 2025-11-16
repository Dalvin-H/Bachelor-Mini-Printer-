#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

const int CS_PIN = 10;           // SD card CS pin
const char* GCODE_FILE = "test.GCO";

uint8_t packageBuf[64];

int buildPackage(const String& cmd, uint8_t* out);
void sendPackage(uint8_t* buf, int len);

// -------------------- Build Package --------------------
int buildPackage(const String& cmd, uint8_t* out) {
    Serial.println("\n--- Building Package ---");
    Serial.println("Raw Gcode: " + cmd);

    uint8_t type = 0x00;
    float X = 0, Y = 0, Z = 0, E = 0, F = 0;

    if (cmd.startsWith("G0")) type = 0x01;
    else if (cmd.startsWith("G1")) type = 0x02;
    else {
        Serial.println("Unsupported command, skipping");
        return 0;
    }

    // Parse values
    int idx = cmd.indexOf('X'); if(idx!=-1) X = cmd.substring(idx+1).toFloat();
    idx = cmd.indexOf('Y'); if(idx!=-1) Y = cmd.substring(idx+1).toFloat();
    idx = cmd.indexOf('Z'); if(idx!=-1) Z = cmd.substring(idx+1).toFloat();
    idx = cmd.indexOf('E'); if(idx!=-1) E = cmd.substring(idx+1).toFloat();
    idx = cmd.indexOf('F'); if(idx!=-1) F = cmd.substring(idx+1).toFloat();

    Serial.print("Parsed: X="); Serial.print(X);
    Serial.print(" Y="); Serial.print(Y);
    Serial.print(" Z="); Serial.print(Z);
    Serial.print(" E="); Serial.print(E);
    Serial.print(" F="); Serial.println(F);

    // Build header
    out[0] = 0xAA;
    out[1] = 0; // length (will set later)
    out[2] = type;
    int offset = 3;

    memcpy(out+offset, &X, 4); offset+=4;
    memcpy(out+offset, &Y, 4); offset+=4;
    memcpy(out+offset, &Z, 4); offset+=4;
    memcpy(out+offset, &E, 4); offset+=4;
    memcpy(out+offset, &F, 4); offset+=4;

    // Set payload length
    out[1] = offset - 2;

    // XOR checksum
    uint8_t cs = 0;
    for(int i=2;i<offset;i++) cs ^= out[i];
    out[offset] = cs;
    offset++;

    Serial.print("Package length: "); Serial.println(offset);
    Serial.print("Checksum: 0x"); if(cs<16) Serial.print('0'); Serial.println(cs, HEX);

    return offset;
}

// -------------------- Send Package --------------------
void sendPackage(uint8_t* buf, int len) {
    Serial.println("Sending package...");
    for(int i=0;i<len;i++){
        if(buf[i]<16) Serial.print('0');
        Serial.print(buf[i],HEX);
        Serial.print(' ');
        Serial.write(buf[i]);
    }
    Serial.println("\nPackage sent.");
}

void setup_SD() {
    Serial.print("Initializing SD card...");

    if (!SD.begin(CS_PIN)) {
        Serial.println("initialization failed!");
        return;
    }
    Serial.println("initialization done.");
}

void read_gcode_file() {
    Serial.println("\n--- Reading G-code File ---");

    File gcodeFile = SD.open(GCODE_FILE);
    if (!gcodeFile) {
        Serial.println("Error opening G-code file!");
        return;
    }

    while(gcodeFile.available()){
        String line = gcodeFile.readStringUntil('\n');
        line.trim();
        if(line.length()==0) continue;

        int len = buildPackage(line, packageBuf);
        if(len>0) sendPackage(packageBuf, len);
    }

    gcodeFile.close();
}



