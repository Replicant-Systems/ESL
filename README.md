# ESL
Electronic shelf label

checks before program to any device
    Check Wi-Fi Credentials
    Check Google Sheets Token

version 1.2 details
1. wifi connection 
2. Fetch data from google sheet and display
3. Short Delay 

version 1.3 details
1. Every day 10.30am data will be updated
2. Controller will be in deep sleep untill 10.30am every day
3. Pin D0 number 16 need to be connected to RST pin directly 
4. This progam not worked because of controller not woke up after deep sleep

version 1.4 details
1. The main logic is in the loop() function.
2. It starts by connecting to Wi-Fi, fetching data from the Google Sheets URL, and displaying it.
3. Then, it connects to the NTP server to get the current time, calculates the delay required to wake up at 10:30 AM, disconnects Wi-Fi, and    enters a delay loop until the next day.