# Project RECALL

Project RECALL is an early-stage Alzheimer’s risk detection device designed to collect speech data throughout a user’s daily life. The device features an e-ink display, microphone, rechargeable battery, and custom hardware that streams voice data to an iPhone for future analysis.

## Why This Was Made

Millions of people are affected by Alzheimer’s disease, and early detection can make a major difference in treatment planning and long-term outcomes. Project RECALL focuses on using speech biomarkers and machine learning to identify early signs of cognitive decline.

To support this research, we needed a way to collect high-quality voice data from patients in a simple, wearable format. The goal of this hardware project is not to run the machine learning model directly on the device, but to create the hardware and data pipeline needed for future analysis.

The device records and transmits speech data in heartbeats to an iPhone, where it can later be processed by our Alzheimer’s detection algorithm.

## Project Goals

The main goals for this prototype were:

* Use a microphone capable of accurately capturing a patient’s voice
* Include an e-ink display to show connection status and patient alerts
* Send voice data to an iPhone for processing
* Maintain long battery life with rechargeable power
* Build a compact and wearable hardware design

## Hardware Overview

The device includes:

* Custom PCB
* Microphone module
* E-ink display
* Rechargeable battery system
* Wireless communication with iPhone
* Off-PCB wiring for supporting components
* 3D printed enclosure

## 3D Model

The 3D model for the device can be found here:

https://cad.onshape.com/documents/2ca69e5fab76ae3f99f92d8b/w/827e84dda2547ca1be5ec7f9/e/8c551b0f0c871be1660480a7?renderMode=0&uiState=6a33286fda4b181361c9916d

## Device Images

<img width="722" height="458" alt="Screenshot 2026-07-23 at 2 48 29 PM" src="https://github.com/user-attachments/assets/be5f8a20-dd63-4599-87c9-a2f53ceb7e2c" />

## PCB Design

<img width="603" height="607" alt="Screenshot 2026-07-23 at 2 49 39 PM" src="https://github.com/user-attachments/assets/05b33b31-1ea3-49ad-ae10-b183145c7b58" />

<img width="719" height="653" alt="Screenshot 2026-07-23 at 2 49 49 PM" src="https://github.com/user-attachments/assets/540baf4f-cace-448e-b0b6-102e2f5b2fac" />



## Wiring Diagram

<img width="584" height="499" alt="Screenshot 2026-07-23 at 2 52 25 PM" src="https://github.com/user-attachments/assets/366d69e3-b14b-4c7a-9d9f-c9e5c2b852db" />


## AI Usage

AI was not significantly used in the hardware design process. However, AI was used to review and debug portions of the firmware. During that review, several firmware issues were identified and corrected, helping improve the reliability of the device.
