long dateTimeToTimestamp(int year, int month, int day, int hour, int minute, int second);
class TimeManager {
  public:
    // Constructor
    TimeManager();

    // Set the time offset
    void setTimeOffset(long offset);

    // Update the current epoch time
    void updateEpoch(long epoch);

    // Calculate the current time
    long getCurrentTime();

  private:
    long _timeOffset;    // User-defined time offset
    long _currentEpoc;   // Epoch time (e.g., from GPS)
    unsigned long _lastUpdate; // Last update time in milliseconds
};

// Constructor
TimeManager::TimeManager() {
  _timeOffset = 0;
  _currentEpoc = 0;
  _lastUpdate = millis();
}

// Set the time offset
void TimeManager::setTimeOffset(long offset) {
  _timeOffset = offset;
}



// Update the current epoch time
void TimeManager::updateEpoch(long epoch) {
  
  if(epoch>_currentEpoc)
  _lastUpdate = millis();
  _currentEpoc = epoch;
}

// Calculate the current time
long TimeManager::getCurrentTime() {
  unsigned long now = millis();
  return _timeOffset + _currentEpoc + ((now - _lastUpdate) / 1000);
}


 long dateTimeToTimestamp(int year, int month, int day, int hour, int minute, int second) {
    // Calculate days since 1970-01-01
    int days = (year - 1970) * 365 + (year - 1969) / 4 - (year - 1900) / 100 + (year - 1600) / 400;

    // Add days for months in the current year
    static const int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    for (int i = 0; i < month - 1; i++) {
        days += daysInMonth[i];
    }

    // Add leap day for February in a leap year
    if (month > 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
        days++;
    }

    // Add days of the current month
    days += day - 1;

    // Convert days to seconds
    unsigned long timestamp = days * 86400UL; // 86400 seconds in a day

    // Add seconds for the current day
    timestamp += hour * 3600UL;   // 3600 seconds in an hour
    timestamp += minute * 60UL;   // 60 seconds in a minute
    timestamp += second;          // Seconds

    return timestamp;
}


void timestampToDateTime(unsigned long timestamp, char* buffer, size_t bufferSize) {
    // Define the number of seconds in various units
    const unsigned long secondsPerMinute = 60;
    const unsigned long secondsPerHour = 3600; // 60 * 60
    const unsigned long secondsPerDay = 86400; // 24 * 3600
    
    // Calculate the number of days since epoch
    unsigned long days = timestamp / secondsPerDay;
    unsigned long secondsOfDay = timestamp % secondsPerDay;
    
    // Calculate the time components
    unsigned long seconds = secondsOfDay % secondsPerMinute;
    unsigned long minutes = (secondsOfDay / secondsPerMinute) % 60;
    unsigned long hours = (secondsOfDay / secondsPerHour) % 24;
    
    // Calculate the year
    int year = 1970;
    while (true) {
        unsigned long daysInYear = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 366 : 365;
        if (days < daysInYear) break;
        days -= daysInYear;
        year++;
    }
    
    // Calculate the month
    static const int daysInMonthNormal[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    static const int daysInMonthLeap[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    
    const int* daysInMonth = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? daysInMonthLeap : daysInMonthNormal;
    int month = 0;
    while (days >= daysInMonth[month]) {
        days -= daysInMonth[month];
        month++;
    }
    
    // Month is 0-indexed, so we need to add 1 for 1-based indexing
    int day = days + 1;
    
    // Format the date and time
    snprintf(buffer, bufferSize, "%04d-%02d-%02d %02d:%02d:%02d", year, month + 1, day, hours, minutes, seconds);
}
