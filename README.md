🎵 QNX Neutrino Metronome Resource Manager

A real-time metronome implemented in C for the QNX Neutrino RTOS, 
using a custom Resource Manager to simulate rhythmic beats via a virtual device /dev/local/metronome.

✨ Features

    🖥 Custom Device Interface — Fully functional resource manager accessible via /dev/local/metronome

    ⏱ Accurate Real-Time Timing — Uses POSIX interval timers to produce beat patterns

    🔄 On-the-Fly Updates — Change BPM and time signatures while running

    ⏸ Mid-Measure Pausing — Resume exactly on the next beat

    🛡 Robust Error Handling — Ignores invalid inputs without crashing

    📝 Acceptance Test Script — Automates functional verification

    📦 Installation & Setup

Prerequisites

    QNX Neutrino RTOS (with Momentics IDE or QNX SDK)

    GCC toolchain for QNX

    Familiarity with QNX resource managers and POSIX APIs

Build
    
    cd Metronome
    make

🚀 Usage
Start the Metronome
    
    ./metronome <bpm> <ts-top> <ts-bottom>


Example:

    ./metronome 120 2 4


This runs the metronome at 120 BPM in 2/4 time.

Available Commands

Command, Example, and Description

    Help -->
        Example: cat /dev/local/metronome-help 
        Description: Show API usage instructions
        
    Status -->	
        Example: cat /dev/local/metronome 
        Description: Show current BPM, time signature, and interval settings
        
    Pause -->	
        Example: echo pause 4 > /dev/local/metronome	
        Description: Pause for 1–9 seconds, resuming on the next beat
        
    Set	echo --> 
        Example: set 200 5 4 > /dev/local/metronome	
        Description: Change BPM and time signature
        
    Stop -->  
        Example: echo stop > /dev/local/metronome	
        Description: Stop beat output (process still runs)
    
    Start -->    
        Example: echo start > /dev/local/metronome	
        Description: Resume from stopped state
    
    Quit -->  
        Example: echo quit > /dev/local/metronome	
        Description: Gracefully terminate the process

🧪 Acceptance Testing

Run the included test script:

    ksh acceptance-test.ksh


Covers:

~ Command parsing & validation

~ BPM/time signature changes

~ Pause behavior

~ Error handling for invalid commands

~ Graceful shutdown


🛠 Architecture Overview: 

    +---------------------------+
    | Resource Manager Thread   |
    | - Parses commands         |
    | - Handles /dev I/O        |
    | - Sends pulses/messages   |
    +-------------+-------------+
                  |
                  v
    +---------------------------+
    | Metronome Timer Thread    |
    | - POSIX interval timers   |
    | - Beat pattern output     |
    | - Pause/resume handling   |
    +---------------------------+

📚 Learning Outcomes

~ Developing QNX resource managers (resmgr_*, iofunc_*)

~ Implementing inter-thread communication with pulses/messages

~ Writing precise real-time scheduling with interval timers

~ Robust POSIX I/O device handling in an embedded environment

~ Applying microkernel principles for reliability and modularity
