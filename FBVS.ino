#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

SoftwareSerial mySerial(8, 9); 
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

struct Voter {
    int id;
    char name[20];  
    char vote;      
};

void setup() {
    pinMode(10,OUTPUT);
    Serial.begin(9600);
    finger.begin(57600);
    delay(1000);

    if (finger.verifyPassword()) {
        Serial.println("Fingerprint sensor found!");
    } else {
        Serial.println("Fingerprint sensor not found!");
        while (1);
    }

    Serial.println("Loading stored voter data...");
    loadStoredData();
}

void loop() {
    int id = getFingerprintID();

    if (id > 0) {
        if (checkIfVoted(id)) {
            Serial.println("You have already voted! Duplicate votes are not allowed.");
        } else {
            Serial.print("Voter Found: ID ");
            Serial.println(id);
            castVote(id);
        }
    } else {
        Serial.println("New Voter Detected! Please Enroll.");
        enrollFingerprint();
    }

    delay(2000);
}

int getFingerprintID() {
    int p;
    Serial.println("Place your finger on the scanner...");

    while (true) {  
        p = finger.getImage();
        if (p == FINGERPRINT_OK) break;
        delay(500);
    }

    Serial.println("Processing fingerprint...");
    p = finger.image2Tz();
    if (p != FINGERPRINT_OK) {
        Serial.println("Failed to convert fingerprint.");
        return -1;
    }

    p = finger.fingerFastSearch();
    if (p != FINGERPRINT_OK) {
        Serial.println("Fingerprint not found in database.");
        return -1;
    }

    Serial.print("Fingerprint Matched! ID: ");
    Serial.println(finger.fingerID);
    return finger.fingerID;
}

int findAvailableID() {
    for (int i = 1; i < 100; i++) {
        if (finger.loadModel(i) != FINGERPRINT_OK) {
            return i;
        }
    }
    return -1; // No available ID found
}

void enrollFingerprint() {
    int voterID = findAvailableID();
    if (voterID == -1) {
        Serial.println("No available ID slots for enrollment.");
        return;
    }

    Serial.print("Assigning ID: ");
    Serial.println(voterID);

    Serial.print("Enter voter name: ");
    while (Serial.available() == 0);
    String voterName = Serial.readString();
    voterName.trim();

    if (storeFingerprint(voterID)) {
        Serial.println("Fingerprint enrolled successfully!");
        saveVoterData(voterID, voterName);
    } else {
        Serial.println("Enrollment failed. Try again.");
    }
}

bool storeFingerprint(int id) {
    int p;
    Serial.println("Place your finger...");

    Serial.println("Waiting for valid fingerprint...");
    while (true) {
        p = finger.getImage();
        if (p == FINGERPRINT_OK) break;
        delay(500);
    }

    p = finger.image2Tz(1);
    if (p != FINGERPRINT_OK) {
        Serial.println("Failed to process fingerprint.");
        return false;
    }

    Serial.println("Remove finger...");
    delay(2000);
    while ((p = finger.getImage()) != FINGERPRINT_NOFINGER);

    Serial.println("Place the same finger again...");
    Serial.println("Waiting for same fingerprint...");
    while (true) {
        p = finger.getImage();
        if (p == FINGERPRINT_OK) break;
        delay(500);
    }

    p = finger.image2Tz(2);
    if (p != FINGERPRINT_OK) {
        Serial.println("Failed to process second scan.");
        return false;
    }

    p = finger.createModel();
    if (p != FINGERPRINT_OK) {
        Serial.println("Failed to create fingerprint model.");
        return false;
    }

    p = finger.storeModel(id);
    if (p != FINGERPRINT_OK) {
        Serial.println("Failed to store fingerprint.");
        return false;
    }
    return true;
}

void castVote(int id) {
    Serial.println("Vote for A or B (Type A or B): ");
    while (Serial.available() == 0);
    String vote = Serial.readString();
    vote.trim();

    if (vote != "A" && vote != "B") {
        Serial.println("Invalid vote. Try again.");
        return;
    }

    Serial.print("You voted for: ");
    Serial.println(vote);

    saveVote(id, vote.charAt(0));

    digitalWrite(10, HIGH);
    delay(1000);
    digitalWrite(10, LOW);
}

// ---------------------- EEPROM STORAGE FUNCTIONS ---------------------- //

void saveVoterData(int id, String name) {
    Voter voter;
    voter.id = id;
    name.toCharArray(voter.name, 20);
    voter.vote = 'N';  // 'N' means no vote cast yet

    EEPROM.put(id * sizeof(Voter), voter);
    Serial.println("Voter data saved to EEPROM.");
}

void saveVote(int id, char vote) {
    Voter voter;
    EEPROM.get(id * sizeof(Voter), voter);
    voter.vote = vote;
    EEPROM.put(id * sizeof(Voter), voter);

    Serial.println("Vote recorded in EEPROM.");
}

bool checkIfVoted(int id) {
    Voter voter;
    EEPROM.get(id * sizeof(Voter), voter);
    return voter.vote == 'A' || voter.vote == 'B';
}

void loadStoredData() {
    for (int i = 0; i < 100; i++) {
        Voter voter;
        EEPROM.get(i * sizeof(Voter), voter);
    }
}
