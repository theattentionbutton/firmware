#include <Arduino.h>

#define MAX_ICON_ID_LEN 20
#define MAX_EMAIL_LEN 255

// Return values
#define SUCCESS 0
#define ERROR_INVALID_FORMAT 1
#define ERROR_TOO_LONG 2

// Function to parse the payload and store results in provided strings
int parse_payload(const String &payload, char *iconId, char *email) {
    // Find the first hash (#) symbol (start of iconId)
    int startIdx = payload.indexOf('#');

    // If there's no hash or not enough data, return error
    if (startIdx == -1) return ERROR_INVALID_FORMAT;

    // Find the next hash (end of iconId, start of email)
    int endIdx = payload.indexOf('#', startIdx + 1);

    // If there's no second hash, return error
    if (endIdx == -1) return ERROR_INVALID_FORMAT;

    // Extract iconId between the hashes
    String extractedIconId = payload.substring(startIdx + 1, endIdx);

    // Truncate iconId to MAX_ICON_ID_LEN
    if (extractedIconId.length() > MAX_ICON_ID_LEN) {
        extractedIconId = extractedIconId.substring(0, MAX_ICON_ID_LEN);
    }

    // Copy the iconId to the provided buffer
    extractedIconId.toCharArray(iconId, MAX_ICON_ID_LEN + 1);

    // Find the last hash (end of email)
    startIdx = payload.indexOf('#', endIdx + 1);

    // If there's no third hash, return error
    if (startIdx == -1) return ERROR_INVALID_FORMAT;

    // Extract email between the hashes
    String extractedEmail = payload.substring(endIdx + 1, startIdx);

    // Truncate email to MAX_EMAIL_LEN
    if (extractedEmail.length() > MAX_EMAIL_LEN) {
        extractedEmail = extractedEmail.substring(0, MAX_EMAIL_LEN);
    }

    // Copy the email to the provided buffer
    extractedEmail.toCharArray(email, MAX_EMAIL_LEN + 1);

    // Return success if everything is okay
    return SUCCESS;
}
