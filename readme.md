# PAKISTAN RAILWAY MANAGEMENT SYSTEM

# PROJECT DOCUMENTATION

## PROJECT OVERVIEW

This project is a console-based Railway Management System developed in C++. It is designed to manage train information, passenger records, ticket reservations, and payment details. The system provides separate functionalities for railway managers and passengers while maintaining all records in text files for permanent storage.

## SYSTEM CAPABILITIES

1. Train Registration -- Add new trains with route details
2. Schedule Display -- View available trains and seat status
3. Passenger Registration -- Store passenger information
4. Ticket Reservation -- Reserve seats on selected trains
5. Secure Payment Module -- Verify card information
6. Ticket Generation -- Display booking details
7. Data Storage -- Save records in files

## AVAILABLE TRAVEL CLASSES

* Business Class
* Parlour Class
* Lower AC Class
* Economy Class

## DATA STORAGE FILES

* `trains.txt` -> Train records
* `passengers.txt` -> Passenger information
* `bookings.txt` -> Reservation records
* `payments.txt` -> Payment transactions

### Record Format

```text
ID, Name, Route, Time, Platform,
Seats, Available Seats, Fare
```

## SAFETY & VALIDATION

* Menu choice validation
* Train schedule conflict checking
* Time format verification
* Card number verification
* Luhn Algorithm implementation
* CVV validation
* Invalid input handling

## PROJECT MODULES

### TRAIN MODULE

* Add train records
* Store route information
* Manage fares and seat capacity
* View train schedules

### PASSENGER MODULE

* Register passengers
* Maintain passenger records

### BOOKING MODULE

* Reserve train seats
* Generate booking records
* Update available seats

### PAYMENT MODULE

* Process card payments
* Validate card information
* Store payment history

### REPORT MODULE

* Generate complete summary
* Display system statistics

## WORK DISTRIBUTION

### Member 1

* Train structures
* Train management functions
* Schedule handling
* Seat and fare management

### Member 2

* Passenger management
* Booking functionality
* Ticket generation
* Search-related functions

### Member 3

* Payment processing
* Card validation algorithms
* File handling operations
* Summary report generation

### Final Integration

* Combined all modules
* Performed testing and debugging
* Verified file storage and outputs

## PROGRAMMING CONCEPTS APPLIED

* Structures
* Arrays
* Functions
* Loops
* Conditional Statements
* File Handling
* Character Arrays
* String Manipulation
* Data Validation
* Time Functions

=========================================================
