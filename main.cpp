/*
 * Bank Management Application
 * C++ OOP-based banking system with file persistence
 * Features: Account creation, deposit, withdrawal, balance check
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <limits>
#include <algorithm>
#include <ctime>

using namespace std;

// ─────────────────────────────────────────────
//  Utility: trim whitespace
// ─────────────────────────────────────────────
static string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    return (start == string::npos) ? "" : s.substr(start, end - start + 1);
}

// ─────────────────────────────────────────────
//  Transaction record
// ─────────────────────────────────────────────
struct Transaction {
    string type;     // "DEPOSIT" | "WITHDRAW" | "OPEN"
    double amount;
    string date;

    string serialize() const {
        return type + "|" + to_string(amount) + "|" + date;
    }

    static Transaction deserialize(const string& line) {
        Transaction t;
        istringstream ss(line);
        string token;
        getline(ss, token, '|'); t.type   = token;
        getline(ss, token, '|'); t.amount = stod(token);
        getline(ss, token, '|'); t.date   = token;
        return t;
    }
};

// ─────────────────────────────────────────────
//  BankAccount class
// ─────────────────────────────────────────────
class BankAccount {
private:
    int    accountNumber;
    string holderName;
    double balance;
    string accountType;   // "SAVINGS" | "CURRENT"
    string createdDate;
    vector<Transaction> history;

    static string currentDate() {
        time_t now = time(nullptr);
        char buf[20];
        strftime(buf, sizeof(buf), "%Y-%m-%d", localtime(&now));
        return string(buf);
    }

public:
    // Constructor for new account
    BankAccount(int accNo, const string& name, double initialDeposit,
                const string& type = "SAVINGS")
        : accountNumber(accNo), holderName(name),
          balance(initialDeposit), accountType(type),
          createdDate(currentDate())
    {
        history.push_back({"OPEN", initialDeposit, createdDate});
    }

    // Constructor for loading from file
    BankAccount() : accountNumber(0), balance(0.0) {}

    // ── Getters ──────────────────────────────
    int         getAccountNumber() const { return accountNumber; }
    string      getHolderName()    const { return holderName;    }
    double      getBalance()       const { return balance;       }
    string      getAccountType()   const { return accountType;   }
    string      getCreatedDate()   const { return createdDate;   }

    // ── Operations ───────────────────────────
    bool deposit(double amount) {
        if (amount <= 0) return false;
        balance += amount;
        history.push_back({"DEPOSIT", amount, currentDate()});
        return true;
    }

    bool withdraw(double amount) {
        if (amount <= 0 || amount > balance) return false;
        balance -= amount;
        history.push_back({"WITHDRAW", amount, currentDate()});
        return true;
    }

    void displayDetails() const {
        cout << "\n  +------------------------------------------+\n";
        cout << "  |         ACCOUNT DETAILS                  |\n";
        cout << "  +------------------------------------------+\n";
        cout << "  | Account No : " << setw(28) << left << accountNumber << "|\n";
        cout << "  | Name       : " << setw(28) << left << holderName    << "|\n";
        cout << "  | Type       : " << setw(28) << left << accountType   << "|\n";
        cout << "  | Balance    : Rs. " << setw(24) << left
             << fixed << setprecision(2) << balance                      << "|\n";
        cout << "  | Opened     : " << setw(28) << left << createdDate   << "|\n";
        cout << "  +------------------------------------------+\n";
    }

    void displayHistory() const {
        cout << "\n  Transaction History for Account #" << accountNumber << "\n";
        cout << "  " << string(54, '-') << "\n";
        cout << "  " << left
             << setw(12) << "Date"
             << setw(12) << "Type"
             << setw(14) << "Amount (Rs.)" << "\n";
        cout << "  " << string(54, '-') << "\n";
        for (const auto& t : history) {
            cout << "  " << left
                 << setw(12) << t.date
                 << setw(12) << t.type
                 << fixed << setprecision(2) << t.amount << "\n";
        }
        cout << "  " << string(54, '-') << "\n";
    }

    // ── Serialization ────────────────────────
    string serialize() const {
        ostringstream oss;
        oss << "ACC:" << accountNumber << "\n"
            << "NAME:" << holderName   << "\n"
            << "BAL:"  << fixed << setprecision(2) << balance << "\n"
            << "TYPE:" << accountType  << "\n"
            << "DATE:" << createdDate  << "\n"
            << "HIST_COUNT:" << history.size() << "\n";
        for (const auto& t : history)
            oss << "HIST:" << t.serialize() << "\n";
        oss << "END\n";
        return oss.str();
    }

    static BankAccount deserialize(istream& in) {
        BankAccount acc;
        string line;
        while (getline(in, line)) {
            line = trim(line);
            if (line == "END") break;
            if (line.rfind("ACC:",  0) == 0) acc.accountNumber = stoi(line.substr(4));
            else if (line.rfind("NAME:", 0) == 0) acc.holderName   = line.substr(5);
            else if (line.rfind("BAL:",  0) == 0) acc.balance      = stod(line.substr(4));
            else if (line.rfind("TYPE:", 0) == 0) acc.accountType  = line.substr(5);
            else if (line.rfind("DATE:", 0) == 0) acc.createdDate  = line.substr(5);
            else if (line.rfind("HIST:", 0) == 0) acc.history.push_back(Transaction::deserialize(line.substr(5)));
        }
        return acc;
    }
};

// ─────────────────────────────────────────────
//  Bank class  (manages all accounts)
// ─────────────────────────────────────────────
class Bank {
private:
    vector<BankAccount> accounts;
    const string DATA_FILE = "bank_data.txt";
    int nextAccNumber;

    void saveToFile() const {
        ofstream out(DATA_FILE);
        if (!out) { cerr << "  [!] Cannot open file for saving.\n"; return; }
        out << "NEXT_ACC:" << nextAccNumber << "\n";
        for (const auto& acc : accounts)
            out << acc.serialize();
        out.close();
    }

    void loadFromFile() {
        ifstream in(DATA_FILE);
        if (!in) { nextAccNumber = 1001; return; }

        string line;
        nextAccNumber = 1001;
        while (getline(in, line)) {
            line = trim(line);
            if (line.rfind("NEXT_ACC:", 0) == 0) {
                nextAccNumber = stoi(line.substr(9));
            } else if (line.rfind("ACC:", 0) == 0) {
                // put the line back by rewinding
                // We use a small trick: seek back
                streampos pos = in.tellg();
                in.seekg(-(streamoff)(line.size() + 1), ios::cur);
                accounts.push_back(BankAccount::deserialize(in));
            }
        }
        in.close();
    }

    BankAccount* findAccount(int accNo) {
        for (auto& acc : accounts)
            if (acc.getAccountNumber() == accNo)
                return &acc;
        return nullptr;
    }

public:
    Bank() { loadFromFile(); }
    ~Bank() { saveToFile(); }

    // ── Create account ───────────────────────
    void createAccount() {
        string name, type;
        double deposit;

        cout << "\n  === Open New Account ===\n";
        cout << "  Enter account holder name : ";
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        getline(cin, name);
        name = trim(name);
        if (name.empty()) { cout << "  [!] Name cannot be empty.\n"; return; }

        cout << "  Account type (1-Savings / 2-Current) : ";
        int ch; cin >> ch;
        type = (ch == 2) ? "CURRENT" : "SAVINGS";

        cout << "  Initial deposit amount (Rs.) : ";
        cin >> deposit;
        if (deposit < 500) {
            cout << "  [!] Minimum initial deposit is Rs. 500.\n";
            return;
        }

        BankAccount newAcc(nextAccNumber, name, deposit, type);
        accounts.push_back(newAcc);
        saveToFile();

        cout << "\n  [+] Account created successfully!\n";
        cout << "  [*] Your Account Number : " << nextAccNumber << "\n";
        nextAccNumber++;
    }

    // ── Deposit ──────────────────────────────
    void deposit() {
        int accNo; double amount;
        cout << "\n  === Deposit ===\n";
        cout << "  Enter account number : "; cin >> accNo;
        BankAccount* acc = findAccount(accNo);
        if (!acc) { cout << "  [!] Account not found.\n"; return; }

        cout << "  Enter deposit amount (Rs.) : "; cin >> amount;
        if (acc->deposit(amount)) {
            saveToFile();
            cout << "  [+] Rs. " << fixed << setprecision(2) << amount
                 << " deposited. New balance: Rs. "
                 << acc->getBalance() << "\n";
        } else {
            cout << "  [!] Invalid amount.\n";
        }
    }

    // ── Withdraw ─────────────────────────────
    void withdraw() {
        int accNo; double amount;
        cout << "\n  === Withdraw ===\n";
        cout << "  Enter account number : "; cin >> accNo;
        BankAccount* acc = findAccount(accNo);
        if (!acc) { cout << "  [!] Account not found.\n"; return; }

        cout << "  Enter withdrawal amount (Rs.) : "; cin >> amount;
        if (acc->withdraw(amount)) {
            saveToFile();
            cout << "  [-] Rs. " << fixed << setprecision(2) << amount
                 << " withdrawn. Remaining balance: Rs. "
                 << acc->getBalance() << "\n";
        } else {
            cout << "  [!] Insufficient balance or invalid amount.\n";
        }
    }

    // ── Balance check ────────────────────────
    void checkBalance() {
        int accNo;
        cout << "\n  === Balance Inquiry ===\n";
        cout << "  Enter account number : "; cin >> accNo;
        BankAccount* acc = findAccount(accNo);
        if (!acc) { cout << "  [!] Account not found.\n"; return; }
        acc->displayDetails();
    }

    // ── Account details ──────────────────────
    void accountDetails() {
        int accNo;
        cout << "\n  Enter account number : "; cin >> accNo;
        BankAccount* acc = findAccount(accNo);
        if (!acc) { cout << "  [!] Account not found.\n"; return; }
        acc->displayDetails();
        acc->displayHistory();
    }

    // ── List all accounts ────────────────────
    void listAllAccounts() {
        if (accounts.empty()) {
            cout << "\n  No accounts found.\n"; return;
        }
        cout << "\n  " << string(60, '=') << "\n";
        cout << "  " << left
             << setw(10) << "Acc. No"
             << setw(22) << "Holder Name"
             << setw(10) << "Type"
             << setw(14) << "Balance (Rs.)" << "\n";
        cout << "  " << string(60, '-') << "\n";
        for (const auto& acc : accounts) {
            cout << "  " << left
                 << setw(10) << acc.getAccountNumber()
                 << setw(22) << acc.getHolderName()
                 << setw(10) << acc.getAccountType()
                 << fixed << setprecision(2) << acc.getBalance() << "\n";
        }
        cout << "  " << string(60, '=') << "\n";
        cout << "  Total accounts : " << accounts.size() << "\n";
    }

    // ── Delete account ───────────────────────
    void deleteAccount() {
        int accNo;
        cout << "\n  === Close Account ===\n";
        cout << "  Enter account number to close : "; cin >> accNo;
        auto it = find_if(accounts.begin(), accounts.end(),
                          [accNo](const BankAccount& a){ return a.getAccountNumber() == accNo; });
        if (it == accounts.end()) { cout << "  [!] Account not found.\n"; return; }

        cout << "  Holder: " << it->getHolderName()
             << "  |  Balance: Rs. " << fixed << setprecision(2) << it->getBalance() << "\n";
        cout << "  Confirm closure? (y/n) : ";
        char ch; cin >> ch;
        if (ch == 'y' || ch == 'Y') {
            accounts.erase(it);
            saveToFile();
            cout << "  [+] Account closed successfully.\n";
        } else {
            cout << "  Cancelled.\n";
        }
    }
};

// ─────────────────────────────────────────────
//  UI helpers
// ─────────────────────────────────────────────
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void printHeader() {
    cout << "\n";
    cout << "  ================================================\n";
    cout << "  |                                              |\n";
    cout << "  |        SECURE BANK MANAGEMENT SYSTEM        |\n";
    cout << "  |            C++ OOP Edition                  |\n";
    cout << "  |                                              |\n";
    cout << "  ================================================\n\n";
}

void printMenu() {
    cout << "  +--------------------------+\n";
    cout << "  |        MAIN MENU         |\n";
    cout << "  +--------------------------+\n";
    cout << "  |  1. Open New Account     |\n";
    cout << "  |  2. Deposit              |\n";
    cout << "  |  3. Withdraw             |\n";
    cout << "  |  4. Balance Inquiry      |\n";
    cout << "  |  5. Account Details      |\n";
    cout << "  |  6. List All Accounts    |\n";
    cout << "  |  7. Close Account        |\n";
    cout << "  |  0. Exit                 |\n";
    cout << "  +--------------------------+\n";
    cout << "  Select option : ";
}

// ─────────────────────────────────────────────
//  Main
// ─────────────────────────────────────────────
int main() {
    Bank bank;
    int choice;

    do {
        clearScreen();
        printHeader();
        printMenu();
        cin >> choice;

        switch (choice) {
            case 1: bank.createAccount();  break;
            case 2: bank.deposit();        break;
            case 3: bank.withdraw();       break;
            case 4: bank.checkBalance();   break;
            case 5: bank.accountDetails(); break;
            case 6: bank.listAllAccounts();break;
            case 7: bank.deleteAccount();  break;
            case 0:
                cout << "\n  Thank you for using Secure Bank. Goodbye!\n\n";
                break;
            default:
                cout << "\n  [!] Invalid option. Please try again.\n";
        }

        if (choice != 0) {
            cout << "\n  Press Enter to continue...";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cin.get();
        }

    } while (choice != 0);

    return 0;
}
