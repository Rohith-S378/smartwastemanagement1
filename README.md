🗑️ Smart Waste Management System

A console-based Smart Waste Management System built in C, featuring multi-role access, fleet management, bin tracking, zone-based scheduling, and report generation — all backed by persistent file storage.


📌 Features


Multi-role access — Separate interfaces for Admin and Civilian users
Bin Management — Track bin fill levels, locations, and status across zones
Fleet & Driver Management — Manage vehicles and assign drivers to collection routes
Zone-based Scheduling — Daily collection schedules organized by zone
Civilian Portal — Register, report waste, and track collection requests
Operations Logging — Log and view waste collection events
Report Generation — Summary reports on collection efficiency and bin status
Persistent Storage — All data saved to .dat and .txt files across sessions
Admin PIN Protection — Secured admin access via PIN authentication



🗂️ Project Structure

smartwastemanagement1/
├── main.c              # Entry point, main menu routing
├── globals.c/h         # Global constants and shared definitions
├── bins.c/h            # Bin data management
├── civilian.c/h        # Civilian registration and requests
├── fleet.c/h           # Vehicle and driver management
├── operations.c/h      # Collection operations and logging
├── reports.c/h         # Report generation
├── utils.c/h           # Utility/helper functions
├── Makefile            # Build configuration
├── admin_pin.dat       # Admin PIN (persistent)
├── bins.dat            # Bin records
├── drivers.dat         # Driver records
├── vehicles.dat        # Vehicle records
├── zones.dat           # Zone definitions
├── collection_log.dat  # Collection event logs
└── daily_schedule.txt  # Generated daily schedule


🛠️ Build & Run

Prerequisites


GCC compiler
Unix/Linux environment (or MinGW on Windows)


Build

bashmake

Run

bash./waste_management

Clean

bashmake clean


👤 User Roles

Admin


Access protected by PIN (admin_pin.dat)
Manage bins, zones, fleet, and drivers
View reports and collection logs
Generate daily schedules


Civilian


Register and log in
Report waste or request collection
View status of nearby bins



💾 Data Persistence

All records are stored in binary .dat files and reloaded on each run, ensuring data persists across sessions without a database.


🧱 Tech Stack

ComponentTechnologyLanguageC (C99)Build SystemMakefileStorageBinary file I/O (.dat)ArchitectureMulti-file modular C


👨‍💻 Author

Rohith S — First-year CSE student at SSN College of Engineering

GitHub: @Rohith-S378


📄 License

This project is open for academic and educational use.
