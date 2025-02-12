//
// Clark Kromenaker
//
// UI for the email subscreen in Sidney.
//
#pragma once
#include <string>
#include <vector>

#include "SidneyMenubar.h"
#include "StringUtil.h"

class Actor;
class PersistState;
class SidneyButton;
class UILabel;
class UIScrollRect;

class SidneyEmail
{
public:
    void Init(Actor* parent, Actor* newEmailParent);

    void Show();
    void Hide();

    void CheckNewEmail();
    void UpdateNewEmail(float deltaTime);

    void OnPersist(PersistState& ps);

private:
    // Root of this subscreen.
    Actor* mRoot = nullptr;

    // The menu bar.
    SidneyMenuBar mMenuBar;

    // The window containing the email list.
    Actor* mEmailListWindow = nullptr;

    // In the email list, labels that have been created to show received emails.
    std::vector<UILabel*> mEmailListItems;

    // The window containing the single email view.
    Actor* mEmailWindow = nullptr;

    // Label to populate with single email data.
    UILabel* mFromLabel = nullptr;
    UILabel* mToLabel = nullptr;
    UILabel* mCCLabel = nullptr;
    UILabel* mDateLabel = nullptr;
    UILabel* mSubjectLabel = nullptr;

    // Single email body labels and scroll rect.
    UIScrollRect* mBodyScrollRect = nullptr;
    std::vector<UILabel*> mBodyLabels;

    // Buttons for actions on an email.
    SidneyButton* mNextButton = nullptr;
    SidneyButton* mPrevButton = nullptr;

    // Represents one email that can be received.
    struct Email
    {
        std::string from;
        std::string to;
        std::string cc;
        std::string date;
        std::string subject;
        std::vector<std::string> body;
    };

    // Contains all emails that can be received and that are known by the game.
    // Key is the email ID specified in SIDNEYEMAIL.TXT.
    std::string_map_ci<Email> mAllEmails;

    // IDs of emails that have been received, in order received.
    std::vector<std::string> mReceivedEmails;

    // Keeps track of which emails we have read.
    std::string_set_ci mReadEmails;

    // For forward/back support, the index of the email we are looking at.
    int mCurrentEmailIndex = -1;

    // New email label.
    UILabel* mNewEmailLabel = nullptr;

    // If true, play "New Email" SFX on next update.
    // Doing this avoids issue where this SFX won't play when you use Action Skip.
    bool mPlayNewEmailSfx = false;

    // Controls "new email" label blink behavior.
    const float kNewEmailBlinkInterval = 0.5f;
    float mNewEmailBlinkTimer = -1.0f;

    void ReceiveEmail(const std::string& emailId);

    void ShowEmailList();
    void ViewEmail(const std::string& emailId, int emailIndex);

    void OnNextEmailButtonPressed();
    void OnPrevEmailButtonPressed();
    void OnReplyButtonPressed();
    void OnComposeButtonPressed();
    void OnPrintButtonPressed();
    void OnCloseButtonPressed();
};