#include <iostream>
#include <cstring>   // strcpy, strcmp, strlen
#include <fstream>   // file read/write
#include <sstream>   // parse comma-separated file lines(helps access 1 line at a time)
#include <cctype>    // isdigit() in CVV, card validation and train time 
using namespace std;

// global variables
const int MAX          = 50;    // max records for any category(e.g global array)
const int MANAGER_CODE = 1234;  // manager login password
const int CLASS_COUNT  = 4;     // number of train classes
struct ClassRule //for classes of train
{
    char  name[15];
    float minFare;
    float maxFare;
};

// Global constant array 
const ClassRule classRules[CLASS_COUNT] = {
    {"Business",  7000.0, 15000.0},
    {"Parlour",   5000.0,  9000.0},
    {"Lower AC",  2500.0,  5500.0},
    {"Economy",    400.0,  1500.0}
};
// STRUCT: TrainClass
// Seats and fare for one class on one train.
struct TrainClass {
    int   seats;      // total seats
    int   available;  // seats not yet booked
    float fare;       // ticket price in PKR
};
// STRUCT: Train
// All details for one train.
// cls[] holds one TrainClass per class (Business/Parlour/etc.)
struct Train {
    char       id[10];
    char       name[30];
    char       from[20];
    char       to[20];
    char       reportTime[10];
    char       departureTime[10];
    int        platform;
    TrainClass cls[CLASS_COUNT];//creates an array for 4 classes in a single train
};
// STRUCT: Passenger
struct Passenger {
    char id[10];
    char name[30];
};
// STRUCT: Booking
// classIdx stores which class was booked (0-3)
struct Booking {
    char id[10];
    char pid[10];   // passenger ID
    char tid[10];   // train ID
    int  classIdx;//line 980,1041 classIdx is assigned number of class name then ci is given class idx value 
    int  seat;
};
// STRUCT: Payment receipt
struct Payment {
    char  bookingId[10];
    char  method[10];
    char  cardNumber[20];  // stored as entered by user
    float amount;
    char  status[10];      // "SUCCESS" or "FAILED"
};
// GLOBAL ARRAYS AND COUNTERS.these are not const since contents of them may change each time
Train     t[MAX];// when we add train e.g green line its stored at t[0],t[1],.....
Passenger p[MAX];//this array stores passengers at p[0]....
Booking   b[MAX];
Payment   pay[MAX];

int tCount   = 0;
int pCount   = 0;
int bCount   = 0;
int payCount = 0;
// INPUT / VALIDATION FUNCTIONS
// Keeps asking until user enters a number between min and max
int getValidChoice(int min, int max) {
    int ch;

    while (true) {
        cin >> ch;
        if (cin.fail()) {
            cin.clear();   //resets up error state
            cin.ignore(10000, '\n');   //ignores garbage value
            cout << "  Invalid! Enter a number: ";
        }
        else {
            if (ch >= min && ch <= max) {
                return ch;
            }
            else {
                cout << "  Invalid! Try again (" << min << "-" << max << "): ";
            }
        }
    }
}

// Returns true if time string is valid HH:MM format
bool validTime(char time[]) {                   //<cstring> for strlen
    if (strlen(time) != 5)  { return false; }   //checks for string length of time, 5 bcz HH:MM
    if (time[2] != ':')     { return false; }   //if retrn false then vaidation error in addTrain()
    if (!isdigit(time[0]) || !isdigit(time[1]) ||     
        !isdigit(time[3]) || !isdigit(time[4])) { return false; } //isdigit via <cctype>
    int hh = (time[0]-'0') * 10 + (time[1]-'0');
    int mm = (time[3]-'0') * 10 + (time[4]-'0');//converts char to num without conversion garbage value 
    return (hh >= 0 && hh <= 23 && mm >= 0 && mm <= 59);
}

// Returns true if another train already uses this platform at this time
bool isConflict(char newTime[], int newPlatform) { //new platform has value of palatform entered by manager
    for (int i = 0; i < tCount; i++) {   //tcount global variable for number of trains counted in loadtrain()
        if (t[i].platform == newPlatform &&  //from line 72 and 37
            strcmp(t[i].departureTime, newTime) == 0) { //==0 means same,strmcp=string compare via <cstring>
            return true; //returns error in addTrain()
        }
    }
    return false;
}

// PAYMENT VALIDATION FUNCTIONS
// These check card format only — no real bank connection.
// Card must be exactly 16 numeric digits
bool validateCardLength(const char card[]) {
    if (strlen(card) != 16) { return false; }
    for (int i = 0; i < 16; i++) {
        if (!isdigit(card[i])) { return false; } //isdigit via <cctype>
    }
    return true;
}

// Luhn algorithm — mathematical checksum all real cards must pass
// Doubles every 2nd digit from right, sum must be divisible by 10
bool luhnCheck(const char card[]) {
    int  sum = 0;
    bool alt = false;
    for (int i = 15; i >= 0; i--) {  //luhn's work left to right
        int n = card[i] - '0';
        if (alt) {
             n *= 2;
             if (n > 9) { n -= 9; } }//if n*2>9 then sub 9
        sum += n;
        alt  = !alt;
    }
    return (sum % 10 == 0);
}

// CVV must be 3 or 4 digits
bool validateCVV(const char cvv[]) {
    int len = strlen(cvv);
    if (len < 3 || len > 4) { return false; }
    for (int i = 0; i < len; i++) {
        if (!isdigit(cvv[i])) { return false; }
    }
    return true;
}

// SEARCH FUNCTIONS
// Linear search — returns array index or -1 if not found
int findTrain(char id[]) {
    for (int i = 0; i < tCount; i++) {// to find train id e.g during booking 
        if (strcmp(t[i].id, id) == 0) { return i; }
    }
    return -1;  //validation on -1 in func call in passengerMenu()
}

int findPassenger(char id[]) {
    for (int i = 0; i < pCount; i++) {
        if (strcmp(p[i].id, id) == 0) { return i; }
    }
    return -1;
}

void savePayment(Payment px) {
    pay[payCount++] = px;          //for 1st payment paycount declared 0
    ofstream fout;
    fout.open("payments.txt", ios::app);  //append mode to prevent overwriting old contents
    fout << px.bookingId << "," << px.method     << ","
         << px.cardNumber << ","
         << px.amount     << "," << px.status    << "\n";
}

// Rewrites entire trains.txt — needed because available seat
// counts change every time a booking is made or train removed
void saveAllTrains() {
    ofstream fout;
    fout.open("trains.txt");
    for (int i = 0; i < tCount; i++) {
        fout<< t[i].id   << "," << t[i].name << ","//global array for train Train t[MAX]
             << t[i].from << "," << t[i].to   << ","
             << t[i].reportTime    << ","
             << t[i].departureTime << ","
             << t[i].platform;
        for (int c = 0; c < CLASS_COUNT; c++) {
            fout << "," << t[i].cls[c].seats
                 << "," << t[i].cls[c].available
                 << "," << t[i].cls[c].fare;
        }
        fout << "\n";
    }
}

void savePassenger(Passenger x) {
   ofstream fout;
    fout.open("passengers.txt", ios::app);
    fout << x.id << "," << x.name << "\n";
}

void saveBooking(Booking x) {
   ofstream fout;
    fout.open("bookings.txt", ios::app);
    fout << x.id  << "," << x.pid      << ","  
         << x.tid << "," << x.classIdx << ","
         << x.seat << "\n";
}
// FILE LOAD FUNCTIONS
// Called once at startup to load all .txt files into memory.
// Count is reset to 0 first to prevent duplicate loading.
// Each line is split by comma using stringstream.
void loadTrains() {
    tCount = 0;
    ifstream fin;
    fin.open("trains.txt");
    if (!fin) { return; }
    string line;
    while (getline(fin, line)) {
        if (line.empty()) { continue; }
        stringstream ss(line); //let's us read the line in small pieces
        string temp;//stores small pieces from line until comma via getline
getline(ss, temp, ','); strcpy(t[tCount].id,    temp.c_str());//temp.c_str()) convert string to char
getline(ss, temp, ','); strcpy(t[tCount].name,          temp.c_str());//copy temp to t[].name
getline(ss, temp, ','); strcpy(t[tCount].from,          temp.c_str());//getline works with string only
getline(ss, temp, ','); strcpy(t[tCount].to,            temp.c_str());//strcpy works for char only
getline(ss, temp, ','); strcpy(t[tCount].reportTime,    temp.c_str());
getline(ss, temp, ','); strcpy(t[tCount].departureTime, temp.c_str());
getline(ss, temp, ',');t[tCount].platform =stoi(temp);//stoi convert string to int,=doesn't work on char
//classes fiels as set by manager
 for (int c = 0; c < CLASS_COUNT; c++) {
 getline(ss, temp, ','); t[tCount].cls[c].seats     = stoi(temp);//stores data for classes of train
getline(ss, temp, ','); t[tCount].cls[c].available = stoi(temp);
getline(ss, temp, ','); t[tCount].cls[c].fare      = stof(temp);
        }
        tCount++;
    }
}

void loadPassengers() {
    pCount = 0;
    ifstream fin;
    fin.open("passengers.txt");
    if (!fin) { return; }
    string line;
    while (getline(fin, line)) {
        if (line.empty()) { continue; }
        stringstream ss(line);
         string temp;
        getline(ss, temp, ','); strcpy(p[pCount].id,   temp.c_str());
        getline(ss, temp, ','); strcpy(p[pCount].name, temp.c_str());
        pCount++;
    }
}

void loadBookings() {
    bCount = 0;
    ifstream fin;
    fin.open("bookings.txt");
    if (!fin) { return; }
    string line;
    while (getline(fin, line)) {
        if (line.empty()) { continue; }
        stringstream ss(line); string temp;
        getline(ss, temp, ','); strcpy(b[bCount].id,  temp.c_str());
        getline(ss, temp, ','); strcpy(b[bCount].pid, temp.c_str());
        getline(ss, temp, ','); strcpy(b[bCount].tid, temp.c_str());
        getline(ss, temp, ','); b[bCount].classIdx=stoi(temp);//classIdx: which class was booked 0-3
        getline(ss, temp, ','); b[bCount].seat     = stoi(temp);
        bCount++;
    }
}

void loadPayments() {
    payCount = 0;
    ifstream fin;
    fin.open("payments.txt");
    if (!fin) { return; }
    string line;
    while (getline(fin, line)) {
        if (line.empty()) { continue; }
        stringstream ss(line); string temp;
        getline(ss, temp, ','); strcpy(pay[payCount].bookingId,  temp.c_str());
        getline(ss, temp, ','); strcpy(pay[payCount].method,     temp.c_str());
        getline(ss, temp, ','); strcpy(pay[payCount].cardNumber, temp.c_str());
        getline(ss, temp, ','); pay[payCount].amount = stof(temp);//stof for float
        getline(ss, temp, ','); strcpy(pay[payCount].status,     temp.c_str());
        payCount++;
    }
}
// FUNCTION: processPayment
// Handles online card payment for passenger bookings.
// 3 attempts allowed. Returns true only if payment approved.
bool processPayment(const char bookingId[], float amount, const char className[]) {
    cout << "\n  ========================================\n";
    cout << "             PAYMENT PORTAL              \n";
    cout << "  ========================================\n";
    cout << "  Class      : " << className << "\n";
    cout << "  Amount Due : PKR " << amount << "\n";
    cout << "  ----------------------------------------\n";
    cout << "  1. Pay by Card\n";
    cout << "  2. Cancel Booking\n";
    cout << "  Enter choice: ";

    if (getValidChoice(1, 2) == 2) {//get valid choice takes input & gives error if not in the range
        cout << "  Booking cancelled.\n";
        return false;//false in passenger menu prints booking not saved
    }

    Payment px;
    strcpy(px.bookingId, bookingId);
    px.amount = amount;
    strcpy(px.method, "CARD");//hardcodes payment method

    char cardRaw[20], cvv[6];
    int  attempts = 3;

    cout << "\n  Test card: 4111111111111111  CVV: 123\n\n";

    while (attempts > 0) {
        cout << "  --- Attempt " << (3- attempts + 1) << " of 3 ---\n";

        cout << "  Card Number (16 digits): ";
        cin >> cardRaw;
        if (!validateCardLength(cardRaw)) {
            cout << "  ERROR: Must be 16 numeric digits.\n";
            cout << "  Attempts left: " << --attempts << "\n\n";//decrements first and then print
            continue;
        }
        if (!luhnCheck(cardRaw)) {
            cout << "  ERROR: Invalid card number. Check for typos.\n";
            cout << "  Attempts left: " << --attempts << "\n\n";
            continue;
        }

        cout << "  CVV: ";
        cin >> cvv;
        if (!validateCVV(cvv)) {
            cout << "  ERROR: CVV must be 3 or 4 digits.\n";
            cout << "  Attempts left: " << --attempts << "\n\n";
            continue;
        }

        // all checks passed — store card number as entered by user
        strcpy(px.cardNumber, cardRaw);
        strcpy(px.status,     "SUCCESS");
        savePayment(px);

        cout << "\n  ========================================\n";
        cout << "           PAYMENT SUCCESSFUL            \n";
        cout << "  ========================================\n";
        cout << "  Card   : " << cardRaw   << "\n";
        cout << "  Class  : " << className << "\n";
        cout << "  Amount : PKR " << amount << "\n";
        cout << "  Status : APPROVED\n";
        cout << "  ========================================\n";
        return true;
    }

    // 3 failed attempts
    strcpy(px.cardNumber, "FAILED");
    strcpy(px.status,     "FAILED");
    savePayment(px);//function call for saving in file
    cout << "\n  Too many failed attempts. Payment DECLINED.\n";
    return false;
}
// FUNCTION: viewTrains
// Displays full schedule with a class table per train.
// Shared by both manager and passenger portals.
void viewTrains() {
    if (tCount == 0) { cout << "  No trains in schedule.\n"; 
        return; }

    cout << "\n================================================================\n";
    cout << "               PAKISTAN RAILWAY SCHEDULE                       \n";
    cout << "================================================================\n";

    for (int i = 0; i < tCount; i++) {
        cout << "\n  [" << i+1 << "] " << t[i].name << "  (" << t[i].id << ")\n";
        cout << "      Route   : " << t[i].from << "  -->  " << t[i].to << "\n";
        cout << "      Departs : " << t[i].departureTime
             << "  Report: "       << t[i].reportTime
             << "  Platform: "     << t[i].platform << "\n";
        cout << "      +-----------+----------------+----------+\n";
        cout << "      | Class     | Seats Avail    | Fare PKR |\n";
        cout << "      +-----------+----------------+----------+\n";
        for (int c = 0; c < CLASS_COUNT; c++) {
            if (t[i].cls[c].seats == 0) { continue; }//if manager sets its seats to 0 then skip
            cout << "      | " << classRules[c].name;
            int pad = 9 - strlen(classRules[c].name);//padding adds spacing to table for orientation 
            for (int j = 0; j < pad; j++) { cout << " "; }
            cout << " | " << t[i].cls[c].available << "/" << t[i].cls[c].seats;//dispays seats xx/yy
int np = 10 - to_string(t[i].cls[c].available).length()//np same as padding but for seat column
-to_string(t[i].cls[c].seats).length();//to_string(t[i].cls[c].available).length()conv int to string 
            for (int j = 0; j < np; j++) { cout << " "; }//conv so that lenght can be measured
            cout << " | " << t[i].cls[c].fare << "    |\n";
        }
        cout << "      +-----------+----------------+----------+\n";
    }

    cout << "\n================================================================\n";
}
// TRAIN MANAGEMENT FUNCTIONS (manager only)

void addTrain() {
    Train &x = t[tCount];//using x as a shortcut term instead of t[tCount]

    cout << "\n-- Add New Train --\n";
    cout << "  Train ID        : "; 
    cin >> x.id;//id just typed by manager
     cin.ignore();

    // reject duplicate train ID
    for (int i = 0; i < tCount; i++) {
        if (strcmp(t[i].id, x.id) == 0) {//string compare
            cout << "  ERROR: Train ID '" << x.id << "' already exists!\n";
            return;
        }
    }

    cout << "  Train Name      : "; cin.getline(x.name, 30);
    cout << "  From            : "; cin.getline(x.from, 20);
    cout << "  To              : "; cin.getline(x.to,   20);

    cout << "  Report Time  (HH:MM): "; cin >> x.reportTime;
    while (!validTime(x.reportTime)) {
        cout << "  Invalid! Enter (HH:MM): "; cin >> x.reportTime;
    }

    cout << "  Departure (HH:MM): "; cin >> x.departureTime;
    while (!validTime(x.departureTime)) {
        cout << "  Invalid! Enter (HH:MM): "; cin >> x.departureTime;
    }

    cout << "  Platform No     : "; cin >> x.platform;
    if (isConflict(x.departureTime, x.platform)) {
        cout << "  ERROR: Platform " << x.platform
             << " already used at "  << x.departureTime << "!\n";
        return;
    }

    // ask seats and fare for all 4 classes in one go
    cout << "\n  Enter seats and fare for each class:\n";
    cout << "  +---+-----------+---------------------+\n";
    cout << "  |   | Class     | Allowed Fare (PKR)  |\n";
    cout << "  +---+-----------+---------------------+\n";
    for (int c = 0; c < CLASS_COUNT; c++) {
        cout << "  | " << c+1 << " | " << classRules[c].name;
        int pad = 9 - strlen(classRules[c].name);
        for (int j = 0; j < pad; j++) { cout << " "; }
        cout << " | " << classRules[c].minFare
             << " - " << classRules[c].maxFare << "         |\n";
    }
    cout << "  +---+-----------+---------------------+\n\n";

    for (int c = 0; c < CLASS_COUNT; c++) {
        cout << "  --- " << classRules[c].name << " ---\n";
        while (true) {
            cin >> x.cls[c].seats;
            if (!cin.fail() && x.cls[c].seats >= 0) { break; }//checks if data type matches or not
            cin.clear(); 
            cin.ignore(10000, '\n');
            cout << "  Invalid! Enter seats: ";
        }
        x.cls[c].available = x.cls[c].seats;//seats entered by manager

        if (x.cls[c].seats == 0) {
            x.cls[c].fare = 0;
            cout << "  (Skipped)\n\n";
            continue;
        }

        cout << "  Fare PKR (" << classRules[c].minFare
             << " - " << classRules[c].maxFare << "): ";
        while (true) {
            cin >> x.cls[c].fare;
            if (cin.fail()) {
                cin.clear(); cin.ignore(10000, '\n');
                cout << "  Invalid: "; continue;
            }
            if (x.cls[c].fare < classRules[c].minFare) {
                cout << "  Min PKR " << classRules[c].minFare << ": "; continue;
            }
            if (x.cls[c].fare > classRules[c].maxFare) {
                cout << "  Max PKR " << classRules[c].maxFare << ": "; continue;
            }
            break;
        }
        cout << "\n";
    }

    tCount++;
    saveAllTrains();

    // confirmation table
    cout << "  Train added successfully!\n";
    cout << "  +-----------+-------+----------+\n";
    cout << "  | Class     | Seats | Fare PKR |\n";
    cout << "  +-----------+-------+----------+\n";
    for (int c = 0; c < CLASS_COUNT; c++) {
        if (x.cls[c].seats == 0) { continue; }
        cout << "  | " << classRules[c].name;
        int pad = 9 - strlen(classRules[c].name);
        for (int j = 0; j < pad; j++) { cout << " "; }
        cout << " | " << x.cls[c].seats << "     | " << x.cls[c].fare << " |\n";
    }
    cout << "  +-----------+-------+----------+\n";
}

void removeTrain() {
    char id[10];
    cout << "  Enter Train ID to remove: "; cin >> id;

    int idx = -1;
    for (int i = 0; i < tCount; i++) {
        if (strcmp(t[i].id, id) == 0) { idx = i; break; }
    }

    if (idx == -1) { cout << "  Train not found!\n"; return; }

    // block removal if active bookings exist
    for (int i = 0; i < bCount; i++) {
        if (strcmp(b[i].tid, id) == 0) {
            cout << "  Cannot remove — active bookings exist for this train.\n";
            return;
        }
    }

    // shift array left to fill the gap
    for (int i = idx; i < tCount - 1; i++) {
        t[i] = t[i+1];
    }

    tCount--;
    saveAllTrains();
    cout << "  Train removed.\n";
}

// PASSENGER MANAGEMENT (manager only)
void addPassenger() {
    Passenger &x = p[pCount];
    cout << "  Passenger ID: ";
     cin >> x.id;

    for (int i = 0; i < pCount; i++) {
        if (strcmp(p[i].id, x.id) == 0) {
            cout << "  ERROR: ID '" << x.id << "' already exists!\n";
            return;
        }
    }

    cin.ignore();
    cout << "  Name        : "; 
    cin.getline(x.name, 30);
    pCount++;
    savePassenger(x);
    cout << "  Passenger saved!\n";
}

// ================================================================
// FUNCTION: printTicketForPassenger
// Privacy-protected ticket print for passenger portal.
// Passenger must enter THEIR OWN passenger ID first.
// Booking ID is then checked to belong to that passenger.
// This prevents anyone from viewing another person's ticket.
// ================================================================
void printTicketForPassenger() {
    char pid[10], bid[10];

    cout << "  Enter your Passenger ID: "; 
    cin >> pid;

    // verify passenger exists
    int pi = findPassenger(pid);
    if (pi == -1) {
        cout << "  Passenger ID not found.\n";
        return;
    }

    cout << "  Enter your Booking ID  : ";
     cin >> bid;

    // find booking that matches BOTH passenger ID and booking ID
    for (int i = 0; i < bCount; i++) {
        if (strcmp(b[i].id,  bid) == 0 &&
            strcmp(b[i].pid, pid) == 0) {

            int ti = findTrain(b[i].tid);
            int ci = b[i].classIdx;//stores class index

            cout << "\n  ============================================\n";
            cout << "          PAKISTAN RAILWAY TICKET            \n";
            cout << "  ============================================\n";
            cout << "  Booking ID : " << b[i].id    << "\n";
            cout << "  Passenger  : " << p[pi].name << "\n";
            if (ti != -1) {
                cout << "  Train      : " << t[ti].name << "\n";
                cout << "  Class      : " << classRules[ci].name << "\n";
                cout << "  Route      : " << t[ti].from << " --> " << t[ti].to << "\n";
                cout << "  Departure  : " << t[ti].departureTime << "\n";
                cout << "  Platform   : " << t[ti].platform      << "\n";
                cout << "  Fare Paid  : PKR " << t[ti].cls[ci].fare << "\n";
            }
            cout << "  Seat No    : " << b[i].seat << "\n";
            cout << "  ============================================\n";
            return;
        }
    }

    // booking ID exists but belongs to a different passenger
    cout << "  No booking found with this ID under your account.\n";
}
// FUNCTION: printTicketForManager
// Manager can look up any booking by ID (no restriction).
// Used for counter operations and dispute resolution.
void printTicketForManager() {
    char bid[10];
    cout << "  Enter Booking ID: "; cin >> bid;

    for (int i = 0; i < bCount; i++) {
        if (strcmp(b[i].id, bid) == 0) {
            int pi = findPassenger(b[i].pid);
            int ti = findTrain(b[i].tid);
            int ci = b[i].classIdx;

            cout << "\n  ============================================\n";
            cout << "          PAKISTAN RAILWAY TICKET            \n";
            cout << "  ============================================\n";
            cout << "  Booking ID : " << b[i].id << "\n";
            cout << "  Passenger  : ";
            if (pi != -1) { cout << p[pi].name << " (ID: " << b[i].pid << ")"; }
            else          { cout << b[i].pid; }
            cout << "\n";
            if (ti != -1) {
                cout << "  Train      : " << t[ti].name << "\n";
                cout << "  Class      : " << classRules[ci].name << "\n";
                cout << "  Route      : " << t[ti].from << " --> " << t[ti].to << "\n";
                cout << "  Departure  : " << t[ti].departureTime << "\n";
                cout << "  Platform   : " << t[ti].platform      << "\n";
                cout << "  Fare Paid  : PKR " << t[ti].cls[ci].fare << "\n";
            }
            cout << "  Seat No    : " << b[i].seat << "\n";
            cout << "  ============================================\n";
            return;
        }
    }
    cout << "  Booking not found!\n";
}
// MANAGER BOOKING (counter booking )
void bookTicketManager() {
    char pid[10], tid[10];
    cout << "  Passenger ID: ";
     cin >> pid;
    cout << "  Train ID    : "; 
    cin >> tid;

    int pi = findPassenger(pid);
    int ti = findTrain(tid);
    if (pi == -1 || ti == -1) { cout << "  Invalid ID!\n"; return; }

    cout << "\n  Available classes:\n";
    bool any = false;
    for (int c = 0; c < CLASS_COUNT; c++) {
        if (t[ti].cls[c].seats == 0) { continue; }
        cout << "  " << c+1 << ". " << classRules[c].name
             << "  Seats: " << t[ti].cls[c].available
             << "/" << t[ti].cls[c].seats
             << "  PKR " << t[ti].cls[c].fare << "\n";
        any = true;
    }
    if (!any) { cout << "  No seats available!\n"; return; }

    cout << "  Select class (1-4): ";
    int ci = getValidChoice(1, 4) - 1;

    if (t[ti].cls[ci].seats     == 0) { cout << "  Class not on this train!\n"; return; }
    if (t[ti].cls[ci].available <= 0) { cout << "  No seats left!\n"; return; }

    Booking &x = b[bCount];
    cout << "  Booking ID: ";
     cin >> x.id;

    // reject duplicate booking ID
    for (int i = 0; i < bCount; i++) {
        if (strcmp(b[i].id, x.id) == 0) {
            cout << "  ERROR: Booking ID already exists!\n"; return;
        }
    }

    // seat selection with duplicate seat check
    int seatNo;
    while (true) {
        cout << "  Seat No (1-" << t[ti].cls[ci].seats << "): ";
         cin >> seatNo;
        if (seatNo < 1 || seatNo > t[ti].cls[ci].seats) {
            cout << "  Invalid seat!\n"; continue;
        }
        bool taken = false;
        for (int i = 0; i < bCount; i++) {//bCount is total bookings
            //b[i].tid ,b[i].classIdx , b[i].seat are of existing bookings
            if (strcmp(b[i].tid, tid) == 0 &&
                b[i].classIdx == ci        &&
                b[i].seat     == seatNo) { taken = true; 
                break; }
        }
        if (taken) {
            cout << "  Seat " << seatNo << " already booked! Choose another.\n";
            continue;
        }
        break;
    }
    x.seat = seatNo;
//copying data used by passenger into bookings struc
    strcpy(x.pid, pid); //due to char data type
    strcpy(x.tid, tid);
    x.classIdx = ci;//classIdx is int therefore = instead of strcpy
    t[ti].cls[ci].available--;
    bCount++;
    saveBooking(x);
    saveAllTrains();

    cout << "  Booking Done! Class: " << classRules[ci].name
         << "  Fare: PKR " << t[ti].cls[ci].fare << "\n";
}
// MENUS
bool managerLogin() {
    int code;
    cout << "\n  *** MANAGER LOGIN ***\n";
    cout << "  Enter Manager Code: "; 
    cin >> code;
    if (code == MANAGER_CODE) 
    { cout << "  Access Granted!\n"; 
        return true; }
    cout << "  Wrong code! Access Denied.\n";
    return false;
}

void trainMenu() {
    int ch;
    do {
        cout << "\n  --- Train Management ---\n";
        cout << "  1. Add Train\n";
        cout << "  2. View Schedule\n";
        cout << "  3. Remove Train\n";
        cout << "  4. Back\n";
        cout << "  Enter choice: ";
        ch = getValidChoice(1, 4);
        if      (ch == 1) { addTrain(); }
        else if (ch == 2) { viewTrains(); }
        else if (ch == 3) { removeTrain(); }
    } while (ch != 4);
}

void managerMenu() {
    if (!managerLogin()) { return; }//exits function
    int ch;
    do {
        cout << "\n  ===== MANAGER PORTAL =====\n";
        cout << "  1. Train Management\n";
        cout << "  2. Add Passenger\n";
        cout << "  3. Book Ticket (Counter)\n";
        cout << "  4. Print Ticket\n";
        cout << "  5. Logout\n";
        cout << "  Enter choice: ";
        ch = getValidChoice(1, 5);
        if      (ch == 1) { trainMenu(); }
        else if (ch == 2) { addPassenger(); }
        else if (ch == 3) { bookTicketManager(); }
        else if (ch == 4) { printTicketForManager(); }
    } while (ch != 5);
    cout << "  Logged out.\n";
}

void passengerMenu() {
    int ch;
    do {
        cout << "\n  ===== PASSENGER PORTAL =====\n";
        cout << "  1. View Train Schedule\n";
        cout << "  2. Book Ticket\n";
        cout << "  3. Print My Ticket\n";
        cout << "  4. Back\n";
        cout << "  Enter choice: ";
        ch = getValidChoice(1, 4);

        if (ch == 1) {
            viewTrains();

        } else if (ch == 2) {

            // identify passenger — register if new
            char pid[10];
            cout << "\n  Enter your Passenger ID (e.g. P001): "; 
            cin >> pid;

            if (findPassenger(pid) == -1) {
                cout << "  Not found. Register? (1=Yes, 2=No): ";
                if (getValidChoice(1, 2) == 1) {
                    Passenger &x = p[pCount];
                    strcpy(x.id, pid);
                    cin.ignore();
                    cout << "  Your Name: "; cin.getline(x.name, 30);
                    pCount++;
                    savePassenger(x);
                    cout << "\n  *** Save your details ***\n";
                    cout << "  Passenger ID : " << pid     << "\n";
                    cout << "  Name         : " << x.name  << "\n";
                    cout << "  *************************\n\n";
                } else { continue; }//if choice==2 then do while loop again operated
            } else {
                // existing passenger — just greet, no re-registration
                cout << "  Welcome back, " << p[findPassenger(pid)].name << "!\n";
            }

            // pick train
            viewTrains();
            char tid[10];
            cout << "  Enter Train ID: "; 
            cin >> tid;
            int ti = findTrain(tid);
            if (ti == -1) { cout << "  Train not found!\n";
             continue; }

            // pick class
            cout << "\n  Available classes:\n";
            bool any = false;
            for (int c = 0; c < CLASS_COUNT; c++) {
                if (t[ti].cls[c].seats == 0) { continue; }
                cout << "  " << c+1 << ". " << classRules[c].name
                     << "  Seats: " << t[ti].cls[c].available
                     << "  Fare: PKR " << t[ti].cls[c].fare << "\n";
                any = true;
            }
         if (!any) { cout << "  No seats available!\n"; continue; }//if all classes have no seats

            cout << "  Select class (1-4): ";
            int ci = getValidChoice(1, 4) - 1;
            if (t[ti].cls[ci].seats     == 0)
         { cout << "  Not on this train!\n"; //when selected class has 0 seats
                continue; }
            if (t[ti].cls[ci].available <= 0) 
            { cout << "  No seats left!\n";   
                  continue; }

            // pick seat — rejects already-booked seats
            int seatNo;
            while (true) {
                cout << "  Seat No (1-" << t[ti].cls[ci].seats << "): ";//if all checks passed
                cin >> seatNo;
                if (seatNo < 1 || seatNo > t[ti].cls[ci].seats) {
                    cout << "  Invalid seat!\n"; continue;
                }
                bool taken = false;
                for (int i = 0; i < bCount; i++) {
                    if (strcmp(b[i].tid, tid) == 0 &&//comparing
                        b[i].classIdx == ci        &&
                        b[i].seat     == seatNo) { taken = true; break; }
                }
                if (taken) {
                    cout << "  Seat " << seatNo << " already booked! Choose another.\n";
                    continue;
                }
                break;
            }

            // get unique booking ID
            char bkId[10];
            while (true) {
                cout << "  Booking ID (e.g. B001): ";
                 cin >> bkId;
                bool exists = false;
                for (int i = 0; i < bCount; i++) {
                    if (strcmp(b[i].id, bkId) == 0) { exists = true; 
                        break; }
                }
                if (exists) {
                    cout << "  ID taken! Enter another.\n"; continue;
                }
                break;
            }

            // show summary before payment
            int pi = findPassenger(pid);
            cout << "\n  ======== Booking Summary ========\n";
            cout << "  Passenger : ";
            if (pi != -1) { cout << p[pi].name; } 
            else { cout << pid; }
            cout << "\n";
            cout << "  Train     : " << t[ti].name          << "\n";
            cout << "  Class     : " << classRules[ci].name << "\n";
            cout << "  Route     : " << t[ti].from << " --> " << t[ti].to << "\n";
            cout << "  Departure : " << t[ti].departureTime << "\n";
            cout << "  Platform  : " << t[ti].platform      << "\n";
            cout << "  Seat      : " << seatNo               << "\n";
            cout << "  Fare      : PKR " << t[ti].cls[ci].fare << "\n";
            cout << "  =================================\n";
            cout << "  Confirm? (1=Yes, 2=No): ";
            if (getValidChoice(1, 2) == 2) 
            { cout << "  Cancelled.\n"; continue; }

            // payment — booking saved only if payment succeeds
bool paid = processPayment(bkId, t[ti].cls[ci].fare, classRules[ci].name);//function call
            if (!paid) { cout << "  Booking NOT saved.\n"; continue; }//if payment unsuccessful

            Booking &bk = b[bCount];
        strcpy(bk.id, bkId);//copy
        strcpy(bk.pid, pid);
        strcpy(bk.tid, tid);//char variable
            bk.classIdx = ci;//int variable
            bk.seat     = seatNo;
            t[ti].cls[ci].available--;
            bCount++;
            saveBooking(bk);
            saveAllTrains();

            cout << "\n  *** Booking Confirmed! ***\n";
            cout << "  Passenger ID : " << pid   << "\n";
            cout << "  Booking ID   : " << bkId  << "\n";
            cout << "  Class        : " << classRules[ci].name    << "\n";
            cout << "  Seat         : " << seatNo                  << "\n";
            cout << "  Fare Paid    : PKR " << t[ti].cls[ci].fare << "\n";
            cout << "  Save both IDs to print your ticket later.\n";

        } else if (ch == 3) {
           
            printTicketForPassenger();
        }

    } while (ch != 4);
}
// MAIN

int main() {
    loadTrains();
    loadPassengers();
    loadBookings();
    loadPayments();

    int ch;
    do {
        cout << "\n  ==============================\n";
        cout << "    PAKISTAN RAILWAY SYSTEM    \n";
        cout << "  ==============================\n";
        cout << "  1. Train Manager\n";
        cout << "  2. Passenger\n";
        cout << "  3. Exit\n";
        cout << "  Enter choice: ";
        ch = getValidChoice(1, 3);
        if      (ch == 1) { managerMenu(); }
        else if (ch == 2) { passengerMenu(); }
    } while (ch != 3);

    cout << "\n  Thank you for using Pakistan Railway System!\n";
    return 0;
}
