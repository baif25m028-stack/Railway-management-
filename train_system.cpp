#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include <cctype>
#include <ctime>
using namespace std;

const int MAX = 50;
const int MANAGER_CODE = 1234;

// ================================================
//                  STRUCTS
// ================================================
struct Train {
    char id[10], name[30], from[20], to[20];
    char reportTime[10], departureTime[10];
    int platform;
    int seats, available;
    char trainClass[15];
    float fare;
};

struct Passenger {
    char id[10], name[30];
};

struct Booking {
    char id[10], pid[10], tid[10];
    int seat;
};

struct Payment {
    char bookingId[10];
    char method[10];
    char cardNumber[25];
    char expiry[8];
    float amount;
    char status[10];
};

// ================================================
//              CLASS FARE RULES
// ================================================
struct ClassRule {
    char name[15];
    float minFare;
    float maxFare;
};

const int CLASS_COUNT = 4;
const ClassRule classRules[CLASS_COUNT] = {
    {"Parlour",  6000.0f, 12000.0f},
    {"Lower AC", 3000.0f,  6000.0f},
    {"Business", 1500.0f,  3500.0f},
    {"Economy",   400.0f,  1500.0f}
};

// ================================================
//                  GLOBAL DATA
// ================================================
Train     t[MAX];
Passenger p[MAX];
Booking   b[MAX];
Payment   pay[MAX];

int tCount = 0, pCount = 0, bCount = 0, payCount = 0;

// ================================================
//            INPUT VALIDATION HELPERS
// ================================================
int getValidChoice(int min, int max) {
    int ch;
    while (true) {
        cin >> ch;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "Invalid! Enter a number: ";
            continue;
        }
        if (ch >= min && ch <= max) return ch;
        cout << "Invalid! Try again (" << min << "-" << max << "): ";
    }
}

bool validTime(char time[]) {
    if (strlen(time) != 5) return false;
    if (time[2] != ':') return false;
    if (!isdigit(time[0]) || !isdigit(time[1]) ||
        !isdigit(time[3]) || !isdigit(time[4])) return false;
    int hh = (time[0]-'0')*10 + (time[1]-'0');
    int mm = (time[3]-'0')*10 + (time[4]-'0');
    return (hh >= 0 && hh <= 23 && mm >= 0 && mm <= 59);
}

bool isConflict(char newTime[], int newPlatform) {
    for (int i = 0; i < tCount; i++)
        if (t[i].platform == newPlatform &&
            strcmp(t[i].departureTime, newTime) == 0)
            return true;
    return false;
}

// ================================================
//          PAYMENT VALIDATION FUNCTIONS
// ================================================
bool validateCardLength(const char card[]) {
    if ((int)strlen(card) != 16) return false;
    for (int i = 0; i < 16; i++)
        if (!isdigit(card[i])) return false;
    return true;
}

bool luhnCheck(const char card[]) {
    int sum = 0;
    bool alt = false;
    for (int i = 15; i >= 0; i--) {
        int n = card[i] - '0';
        if (alt) { n *= 2; if (n > 9) n -= 9; }
        sum += n;
        alt = !alt;
    }
    return (sum % 10 == 0);
}

bool validateExpiry(const char expiry[]) {
    if ((int)strlen(expiry) != 5) return false;
    if (expiry[2] != '/') return false;
    if (!isdigit(expiry[0]) || !isdigit(expiry[1]) ||
        !isdigit(expiry[3]) || !isdigit(expiry[4])) return false;
    int mm = (expiry[0]-'0')*10 + (expiry[1]-'0');
    int yy = (expiry[3]-'0')*10 + (expiry[4]-'0');
    if (mm < 1 || mm > 12) return false;
    time_t now = time(0);
    tm* ltm = localtime(&now);
    int curYY = ltm->tm_year % 100;
    int curMM = ltm->tm_mon + 1;
    if (yy < curYY) return false;
    if (yy == curYY && mm < curMM) return false;
    return true;
}

bool validateCVV(const char cvv[]) {
    int len = strlen(cvv);
    if (len < 3 || len > 4) return false;
    for (int i = 0; i < len; i++)
        if (!isdigit(cvv[i])) return false;
    return true;
}

void maskCard(const char card[], char masked[]) {
    strcpy(masked, "****-****-****-");
    masked[15] = card[12]; masked[16] = card[13];
    masked[17] = card[14]; masked[18] = card[15];
    masked[19] = '\0';
}

// ================================================
//                FILE SAVE
// ================================================
void savePayment(Payment px) {
    ofstream file("payments.txt", ios::app);
    file << px.bookingId << "," << px.method << ","
         << px.cardNumber << "," << px.expiry << ","
         << px.amount << "," << px.status << "\n";
}

void saveAllTrains() {
    ofstream file("trains.txt");
    for (int i = 0; i < tCount; i++)
        file << t[i].id << "," << t[i].name << ","
             << t[i].from << "," << t[i].to << ","
             << t[i].reportTime << "," << t[i].departureTime << ","
             << t[i].platform << "," << t[i].seats << ","
             << t[i].available << "," << t[i].trainClass << ","
             << t[i].fare << "\n";
}

void saveTrain(Train x) {
    ofstream file("trains.txt", ios::app);
    file << x.id << "," << x.name << ","
         << x.from << "," << x.to << ","
         << x.reportTime << "," << x.departureTime << ","
         << x.platform << "," << x.seats << ","
         << x.available << "," << x.trainClass << ","
         << x.fare << "\n";
}

void savePassenger(Passenger x) {
    ofstream file("passengers.txt", ios::app);
    file << x.id << "," << x.name << "\n";
}

void saveBooking(Booking x) {
    ofstream file("bookings.txt", ios::app);
    file << x.id << "," << x.pid << "," << x.tid << "," << x.seat << "\n";
}

// ================================================
//                FILE LOAD
// ================================================
void loadTrains() {
    ifstream file("trains.txt");
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string temp;
        getline(ss, temp, ','); strcpy(t[tCount].id,            temp.c_str());
        getline(ss, temp, ','); strcpy(t[tCount].name,          temp.c_str());
        getline(ss, temp, ','); strcpy(t[tCount].from,          temp.c_str());
        getline(ss, temp, ','); strcpy(t[tCount].to,            temp.c_str());
        getline(ss, temp, ','); strcpy(t[tCount].reportTime,    temp.c_str());
        getline(ss, temp, ','); strcpy(t[tCount].departureTime, temp.c_str());
        getline(ss, temp, ','); t[tCount].platform  = stoi(temp);
        getline(ss, temp, ','); t[tCount].seats     = stoi(temp);
        getline(ss, temp, ','); t[tCount].available = stoi(temp);
        getline(ss, temp, ','); strcpy(t[tCount].trainClass, temp.c_str());
        getline(ss, temp, ','); t[tCount].fare = temp.empty() ? 500.0f : stof(temp);
        tCount++;
    }
}

void loadPassengers() {
    ifstream file("passengers.txt");
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string temp;
        getline(ss, temp, ','); strcpy(p[pCount].id,   temp.c_str());
        getline(ss, temp, ','); strcpy(p[pCount].name, temp.c_str());
        pCount++;
    }
}

void loadBookings() {
    ifstream file("bookings.txt");
    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string temp;
        getline(ss, temp, ','); strcpy(b[bCount].id,  temp.c_str());
        getline(ss, temp, ','); strcpy(b[bCount].pid, temp.c_str());
        getline(ss, temp, ','); strcpy(b[bCount].tid, temp.c_str());
        getline(ss, temp, ','); b[bCount].seat = stoi(temp);
        bCount++;
    }
}

// ================================================
//              SEARCH HELPERS
// ================================================
int findTrain(char id[]) {
    for (int i = 0; i < tCount; i++)
        if (strcmp(t[i].id, id) == 0) return i;
    return -1;
}

int findPassenger(char id[]) {
    for (int i = 0; i < pCount; i++)
        if (strcmp(p[i].id, id) == 0) return i;
    return -1;
}

// ================================================
//              PAYMENT FLOW
// ================================================
bool processPayment(const char bookingId[], float amount, const char trainClass[]) {

    cout << "\n  ========================================\n";
    cout << "             PAYMENT PORTAL              \n";
    cout << "  ========================================\n";
    cout << "  Class      : " << trainClass << "\n";
    cout << "  Amount Due : PKR " << amount << "\n";
    cout << "  ----------------------------------------\n";
    cout << "  1. Credit / Debit Card\n";
    cout << "  2. Cash at Counter\n";
    cout << "  3. Cancel Booking\n";
    cout << "  Enter choice: ";
    int method = getValidChoice(1, 3);

    if (method == 3) { cout << "  Booking cancelled.\n"; return false; }

    Payment px;
    strcpy(px.bookingId, bookingId);
    px.amount = amount;

    // ---------- CASH ----------
    if (method == 2) {
        strcpy(px.method,     "CASH");
        strcpy(px.cardNumber, "N/A");
        strcpy(px.expiry,     "N/A");
        strcpy(px.status,     "SUCCESS");
        savePayment(px);
        payCount++;
        cout << "\n  Please pay PKR " << amount << " at the ticket counter.\n";
        cout << "  Cash payment recorded. Booking confirmed!\n";
        return true;
    }

    // ---------- CARD ----------
    strcpy(px.method, "CARD");
    char cardRaw[20], expiry[8], cvv[6];
    int attempts = 3;

    cout << "\n  Test card numbers that work:\n";
    cout << "  4111111111111111  |  Expiry: 12/26  |  CVV: 123\n";
    cout << "  5425233430109903  |  Expiry: 09/27  |  CVV: 321\n\n";

    while (attempts > 0) {
        cout << "  --- Card Details (Attempt " << (4 - attempts) + 1 << " of 3) ---\n";

        // Card number
        cout << "  Card Number (16 digits, no spaces): ";
        cin >> cardRaw;
        if (!validateCardLength(cardRaw)) {
            cout << "  ERROR: Must be exactly 16 numeric digits.\n";
            cout << "  Attempts left: " << --attempts << "\n\n";
            continue;
        }
        if (!luhnCheck(cardRaw)) {
            cout << "  ERROR: Card number failed checksum. Check for typos.\n";
            cout << "  Attempts left: " << --attempts << "\n\n";
            continue;
        }

        // Expiry
        cout << "  Expiry Date (MM/YY): ";
        cin >> expiry;
        if (!validateExpiry(expiry)) {
            cout << "  ERROR: Invalid or expired date. Use MM/YY e.g. 08/27\n";
            cout << "  Attempts left: " << --attempts << "\n\n";
            continue;
        }

        // CVV
        cout << "  CVV (3 or 4 digits): ";
        cin >> cvv;
        if (!validateCVV(cvv)) {
            cout << "  ERROR: CVV must be 3 or 4 numeric digits.\n";
            cout << "  Attempts left: " << --attempts << "\n\n";
            continue;
        }

        // All passed
        char masked[21];
        maskCard(cardRaw, masked);
        strcpy(px.cardNumber, masked);
        strcpy(px.expiry,     expiry);
        strcpy(px.status,     "SUCCESS");
        savePayment(px);
        payCount++;

        cout << "\n  ========================================\n";
        cout << "           PAYMENT SUCCESSFUL            \n";
        cout << "  ========================================\n";
        cout << "  Card    : " << masked  << "\n";
        cout << "  Expiry  : " << expiry  << "\n";
        cout << "  Class   : " << trainClass << "\n";
        cout << "  Amount  : PKR " << amount << "\n";
        cout << "  Status  : APPROVED (Simulated)\n";
        cout << "  ========================================\n";
        return true;
    }

    // Out of attempts
    strcpy(px.cardNumber, "FAILED");
    strcpy(px.expiry,     "N/A");
    strcpy(px.status,     "FAILED");
    savePayment(px);
    cout << "\n  Too many failed attempts. Payment DECLINED.\n";
    return false;
}

// ================================================
//         CLASS SELECTION (used in addTrain)
// ================================================
int pickClass() {
    cout << "\n  Select Train Class:\n";
    cout << "  +----+-----------+------------------+\n";
    cout << "  | No | Class     | Fare Range (PKR) |\n";
    cout << "  +----+-----------+------------------+\n";
    for (int i = 0; i < CLASS_COUNT; i++) {
        cout << "  |  " << i+1 << " | ";
        // pad class name to 9 chars
        cout << classRules[i].name;
        int pad = 9 - strlen(classRules[i].name);
        for (int j = 0; j < pad; j++) cout << " ";
        cout << " | ";
        cout << classRules[i].minFare << " - " << classRules[i].maxFare;
        cout << "    |\n";
    }
    cout << "  +----+-----------+------------------+\n";
    cout << "  Enter choice: ";
    return getValidChoice(1, CLASS_COUNT) - 1;
}

// ================================================
//            TRAIN MANAGEMENT
// ================================================
void addTrain() {
    Train &x = t[tCount];

    cout << "\n-- Add New Train --\n";
    cout << "Train ID   : "; cin >> x.id; cin.ignore();
    cout << "Train Name : "; cin.getline(x.name, 30);
    cout << "From       : "; cin.getline(x.from, 20);
    cout << "To         : "; cin.getline(x.to, 20);

    cout << "Report Time (HH:MM)   : "; cin >> x.reportTime;
    while (!validTime(x.reportTime)) {
        cout << "Invalid! Enter (HH:MM): "; cin >> x.reportTime;
    }

    cout << "Departure Time (HH:MM): "; cin >> x.departureTime;
    while (!validTime(x.departureTime)) {
        cout << "Invalid! Enter (HH:MM): "; cin >> x.departureTime;
    }

    cout << "Platform No: "; cin >> x.platform;
    if (isConflict(x.departureTime, x.platform)) {
        cout << "ERROR: Another train is using Platform " << x.platform
             << " at " << x.departureTime << ". Train NOT added.\n";
        return;
    }

    cout << "Total Seats: "; cin >> x.seats;
    x.available = x.seats;

    // --- Pick class ---
    int ci = pickClass();
    strcpy(x.trainClass, classRules[ci].name);

    float minF = classRules[ci].minFare;
    float maxF = classRules[ci].maxFare;

    cout << "\n  Class selected : " << x.trainClass << "\n";
    cout << "  Allowed fare   : PKR " << minF << " to PKR " << maxF << "\n";

    // --- Set fare with min/max check ---
    while (true) {
        cout << "  Enter Ticket Fare (PKR): ";
        cin >> x.fare;
        if (cin.fail()) {
            cin.clear(); cin.ignore(10000, '\n');
            cout << "  Invalid input! Enter a number.\n";
            continue;
        }
        if (x.fare < minF) {
            cout << "  ERROR: Fare too low! Minimum for "
                 << x.trainClass << " is PKR " << minF << "\n";
            continue;
        }
        if (x.fare > maxF) {
            cout << "  ERROR: Fare too high! Maximum for "
                 << x.trainClass << " is PKR " << maxF << "\n";
            continue;
        }
        break;
    }

    tCount++;
    saveTrain(x);

    cout << "\n  Train Added Successfully!\n";
    cout << "  Name   : " << x.name << "\n";
    cout << "  Class  : " << x.trainClass << "\n";
    cout << "  Fare   : PKR " << x.fare << "\n";
}

void viewTrains() {
    if (tCount == 0) { cout << "No trains in schedule.\n"; return; }

    cout << "\n================================================================\n";
    cout << "                   PAKISTAN RAILWAY SCHEDULE                   \n";
    cout << "================================================================\n";
    for (int i = 0; i < tCount; i++) {
        cout << "\n  [" << i+1 << "] " << t[i].name << "  (" << t[i].id << ")\n";
        cout << "      Route    : " << t[i].from << "  -->  " << t[i].to << "\n";
        cout << "      Class    : " << t[i].trainClass << "\n";
        cout << "      Fare     : PKR " << t[i].fare << "\n";
        cout << "      Departs  : " << t[i].departureTime
             << "  |  Report: " << t[i].reportTime
             << "  |  Platform: " << t[i].platform << "\n";
        cout << "      Seats    : " << t[i].available
             << " available / " << t[i].seats << " total\n";
    }
    cout << "\n================================================================\n";
}

void removeTrain() {
    char id[10];
    cout << "Enter Train ID to remove: "; cin >> id;

    int idx = -1;
    for (int i = 0; i < tCount; i++)
        if (strcmp(t[i].id, id) == 0) { idx = i; break; }

    if (idx == -1) { cout << "Train not found!\n"; return; }

    for (int i = 0; i < bCount; i++)
        if (strcmp(b[i].tid, id) == 0) {
            cout << "Cannot remove! Active bookings exist for this train.\n";
            return;
        }

    for (int i = idx; i < tCount-1; i++) t[i] = t[i+1];
    tCount--;
    saveAllTrains();
    cout << "Train removed successfully!\n";
}

// ================================================
//           PASSENGER MANAGEMENT
// ================================================
void addPassenger() {
    Passenger &x = p[pCount];
    cout << "Passenger ID: "; cin >> x.id; cin.ignore();
    cout << "Name        : "; cin.getline(x.name, 30);
    pCount++;
    savePassenger(x);
    cout << "Passenger saved!\n";
}

// ================================================
//            PRINT TICKET
// ================================================
void printTicket() {
    char id[10];
    cout << "Enter Booking ID: "; cin >> id;

    for (int i = 0; i < bCount; i++) {
        if (strcmp(b[i].id, id) == 0) {
            int pi = findPassenger(b[i].pid);
            int ti = findTrain(b[i].tid);

            cout << "\n  ============================================\n";
            cout << "            PAKISTAN RAILWAY TICKET          \n";
            cout << "  ============================================\n";
            cout << "  Booking ID : " << b[i].id << "\n";
            cout << "  Passenger  : ";
            if (pi != -1) cout << p[pi].name << " (" << b[i].pid << ")";
            else cout << b[i].pid;
            cout << "\n";
            if (ti != -1) {
                cout << "  Train      : " << t[ti].name << " (" << b[i].tid << ")\n";
                cout << "  Class      : " << t[ti].trainClass << "\n";
                cout << "  Route      : " << t[ti].from << " --> " << t[ti].to << "\n";
                cout << "  Departure  : " << t[ti].departureTime << "\n";
                cout << "  Platform   : " << t[ti].platform << "\n";
                cout << "  Fare Paid  : PKR " << t[ti].fare << "\n";
            }
            cout << "  Seat No    : " << b[i].seat << "\n";
            cout << "  ============================================\n";
            return;
        }
    }
    cout << "Booking not found!\n";
}

// ================================================
//        MANAGER BOOKING (cash, no payment step)
// ================================================
void bookTicketManager() {
    char pid[10], tid[10];
    cout << "Passenger ID: "; cin >> pid;
    cout << "Train ID    : "; cin >> tid;

    int pi = findPassenger(pid);
    int ti = findTrain(tid);
    if (pi == -1 || ti == -1) { cout << "Invalid ID!\n"; return; }
    if (t[ti].available <= 0) { cout << "No seats available!\n"; return; }

    Booking &x = b[bCount];
    cout << "Booking ID  : "; cin >> x.id;
    cout << "Seat No     : "; cin >> x.seat;
    if (x.seat < 1 || x.seat > t[ti].seats) { cout << "Invalid seat!\n"; return; }

    strcpy(x.pid, pid);
    strcpy(x.tid, tid);
    t[ti].available--;
    bCount++;
    saveBooking(x);
    saveAllTrains();

    cout << "Booking Done! Class: " << t[ti].trainClass
         << " | Fare: PKR " << t[ti].fare << "\n";
}

// ================================================
//              MANAGER PORTAL
// ================================================
bool managerLogin() {
    int code;
    cout << "\n  *** MANAGER LOGIN ***\n";
    cout << "  Enter Manager Code: "; cin >> code;
    if (code == MANAGER_CODE) { cout << "  Access Granted!\n"; return true; }
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
        if      (ch == 1) addTrain();
        else if (ch == 2) viewTrains();
        else if (ch == 3) removeTrain();
    } while (ch != 4);
}

void managerMenu() {
    if (!managerLogin()) return;
    int ch;
    do {
        cout << "\n  ===== MANAGER PORTAL =====\n";
        cout << "  1. Train Management\n";
        cout << "  2. Add Passenger\n";
        cout << "  3. Book Ticket (Counter)\n";
        cout << "  4. Print Ticket\n";
        cout << "  5. View Train Schedule\n";
        cout << "  6. Logout\n";
        cout << "  Enter choice: ";
        ch = getValidChoice(1, 6);
        if      (ch == 1) trainMenu();
        else if (ch == 2) addPassenger();
        else if (ch == 3) bookTicketManager();
        else if (ch == 4) printTicket();
        else if (ch == 5) viewTrains();
    } while (ch != 6);
    cout << "  Logged out from Manager Portal.\n";
}

// ================================================
//          PASSENGER PORTAL (with payment)
// ================================================
void passengerMenu() {
    int ch;
    do {
        cout << "\n  ===== PASSENGER PORTAL =====\n";
        cout << "  1. View Train Schedule\n";
        cout << "  2. Book Ticket\n";
        cout << "  3. Print Ticket\n";
        cout << "  4. Back\n";
        cout << "  Enter choice: ";
        ch = getValidChoice(1, 4);

        if (ch == 1) {
            viewTrains();

        } else if (ch == 2) {

            // Step 1: Identify passenger
            char pid[10];
            cout << "\n  Enter your Passenger ID: "; cin >> pid;

            if (findPassenger(pid) == -1) {
                cout << "  ID not found. Register now? (1=Yes, 2=No): ";
                int reg = getValidChoice(1, 2);
                if (reg == 1) {
                    Passenger &x = p[pCount];
                    strcpy(x.id, pid);
                    cin.ignore();
                    cout << "  Enter your Name: "; cin.getline(x.name, 30);
                    pCount++;
                    savePassenger(x);
                    cout << "  Registered successfully!\n";
                } else continue;
            }

            // Step 2: Show schedule and pick train
            viewTrains();
            char tid[10];
            cout << "  Enter Train ID: "; cin >> tid;
            int ti = findTrain(tid);
            if (ti == -1) { cout << "  Train not found!\n"; continue; }
            if (t[ti].available <= 0) { cout << "  No seats available!\n"; continue; }

            // Step 3: Pick seat
            cout << "  Enter Seat No (1-" << t[ti].seats << "): ";
            int seatNo; cin >> seatNo;
            if (seatNo < 1 || seatNo > t[ti].seats) {
                cout << "  Invalid seat number!\n"; continue;
            }

            // Step 4: Booking ID
            char bkId[10];
            cout << "  Enter Booking ID: "; cin >> bkId;

            // Step 5: Show full summary before payment
            int pi = findPassenger(pid);
            cout << "\n  ======== Booking Summary ========\n";
            cout << "  Passenger  : ";
            if (pi != -1) cout << p[pi].name << " (" << pid << ")";
            else cout << pid;
            cout << "\n";
            cout << "  Train      : " << t[ti].name << " (" << tid << ")\n";
            cout << "  Class      : " << t[ti].trainClass << "\n";
            cout << "  Route      : " << t[ti].from << " --> " << t[ti].to << "\n";
            cout << "  Departure  : " << t[ti].departureTime << "\n";
            cout << "  Platform   : " << t[ti].platform << "\n";
            cout << "  Seat No    : " << seatNo << "\n";
            cout << "  Fare       : PKR " << t[ti].fare << "\n";
            cout << "  =================================\n";
            cout << "  Confirm and proceed to payment? (1=Yes, 2=No): ";
            if (getValidChoice(1, 2) == 2) { cout << "  Booking cancelled.\n"; continue; }

            // Step 6: Payment (passes class name for display)
            bool paid = processPayment(bkId, t[ti].fare, t[ti].trainClass);
            if (!paid) { cout << "  Booking was NOT saved.\n"; continue; }

            // Step 7: Save booking only after payment success
            Booking &bk = b[bCount];
            strcpy(bk.id,  bkId);
            strcpy(bk.pid, pid);
            strcpy(bk.tid, tid);
            bk.seat = seatNo;
            t[ti].available--;
            bCount++;
            saveBooking(bk);
            saveAllTrains();

            cout << "\n  *** Booking Confirmed! ***\n";
            cout << "  Booking ID : " << bkId << "\n";
            cout << "  Class      : " << t[ti].trainClass << "\n";
            cout << "  Seat       : " << seatNo << "\n";
            cout << "  Fare Paid  : PKR " << t[ti].fare << "\n";

        } else if (ch == 3) {
            printTicket();
        }

    } while (ch != 4);
}

// ================================================
//                    MAIN
// ================================================
int main() {
    loadTrains();
    loadPassengers();
    loadBookings();

    int ch;
    do {
        cout << "\n  ==============================\n";
        cout << "   PAKISTAN RAILWAY SYSTEM    \n";
        cout << "  ==============================\n";
        cout << "  1. Train Manager\n";
        cout << "  2. Passenger\n";
        cout << "  3. Exit\n";
        cout << "  Enter choice: ";
        ch = getValidChoice(1, 3);
        if      (ch == 1) managerMenu();
        else if (ch == 2) passengerMenu();
    } while (ch != 3);

    cout << "\n  Thank you for using Pakistan Railway System!\n";
    return 0;
}