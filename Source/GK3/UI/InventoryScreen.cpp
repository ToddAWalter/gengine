#include "InventoryScreen.h"

#include "ActionBar.h"
#include "ActionManager.h"
#include "AssetManager.h"
#include "InputManager.h"
#include "InventoryManager.h"
#include "RectTransform.h"
#include "ReportManager.h"
#include "StringUtil.h"
#include "Texture.h"
#include "UIButton.h"
#include "UICanvas.h"
#include "UIImage.h"
#include "Window.h"

InventoryScreen::InventoryScreen() : Actor("InventoryScreen", TransformType::RectTransform),
    mLayer("InventoryLayer")
{
    // Inventory overrides SFX/VO, but continues ambient audio from scene.
    mLayer.OverrideAudioState(true, true, false);

    // Add canvas for rendering UI elements.
	AddComponent<UICanvas>(5);

	// Add translucent background image that tints the scene.
	UIImage* background = AddComponent<UIImage>();
	background->SetTexture(&Texture::Black);
	background->SetColor(Color32(0, 0, 0, 128));

	RectTransform* inventoryRectTransform = GetComponent<RectTransform>();
	inventoryRectTransform->SetSizeDelta(0.0f, 0.0f);
	inventoryRectTransform->SetAnchorMin(Vector2::Zero);
	inventoryRectTransform->SetAnchorMax(Vector2::One);

	// Add exit button to bottom-left corner of screen.
	Actor* exitButtonActor = new Actor(TransformType::RectTransform);
    exitButtonActor->GetTransform()->SetParent(GetTransform());
	UIButton* exitButton = exitButtonActor->AddComponent<UIButton>();

	exitButton->SetUpTexture(gAssetManager.LoadTexture("EXITN.BMP"));
	exitButton->SetDownTexture(gAssetManager.LoadTexture("EXITD.BMP"));
	exitButton->SetHoverTexture(gAssetManager.LoadTexture("EXITHOV.BMP"));
	exitButton->SetDisabledTexture(gAssetManager.LoadTexture("EXITDIS.BMP"));
    exitButton->SetPressCallback([this](UIButton* button) {
        Hide();
    });
    exitButton->SetTooltipText("inventoryexit");
    mExitButton = exitButton;

	RectTransform* exitButtonRectTransform = exitButtonActor->GetComponent<RectTransform>();
	exitButtonRectTransform->SetParent(inventoryRectTransform);
	exitButtonRectTransform->SetSizeDelta(58.0f, 26.0f); // texture width/height
	exitButtonRectTransform->SetAnchor(Vector2::Zero);
	exitButtonRectTransform->SetAnchoredPosition(10.0f, 10.0f);
	exitButtonRectTransform->SetPivot(0.0f, 0.0f);

	// Create active inventory item highlight, but hide by default.
	Actor* activeHighlightActor = new Actor(TransformType::RectTransform);
    activeHighlightActor->GetTransform()->SetParent(GetTransform());
	mActiveHighlightImage = activeHighlightActor->AddComponent<UIImage>();
	mActiveHighlightImage->SetTexture(gAssetManager.LoadTexture("INV_HIGHLIGHT.BMP"), true);
	mActiveHighlightImage->SetEnabled(false);

	RectTransform* activeHighlightRectTransform = mActiveHighlightImage->GetRectTransform();
	activeHighlightRectTransform->SetParent(inventoryRectTransform);
	activeHighlightRectTransform->SetAnchor(0.0f, 1.0f);
	activeHighlightRectTransform->SetPivot(0.0f, 1.0f);

	// Hide inventory UI by default.
    SetActive(false);
}

void InventoryScreen::Show(const std::string& actorName, const std::set<std::string>& inventory)
{
    // Already showing, so don't do it again!
    if(IsActive()) { return; }

    // Push layer onto stack.
    gLayerManager.PushLayer(&mLayer);

    // Save current actor name and inventory.
    mCurrentActorName = actorName;
    mCurrentInventory = &inventory;

    // Layout the buttons on screen.
    RefreshLayout();

	// Actually show the stuff!
	SetActive(true);
}

void InventoryScreen::Hide()
{
    if(!IsActive()) { return; }
	SetActive(false);

    // Pop off stack.
    gLayerManager.PopLayer(&mLayer);

    // Clear inventory data.
    mCurrentActorName.clear();
    mCurrentInventory = nullptr;
}

bool InventoryScreen::IsShowing() const
{
    return gLayerManager.IsTopLayer(&mLayer);
}

void InventoryScreen::RefreshLayout()
{
    // Hide active inventory highlight by default.
    mActiveHighlightImage->SetEnabled(false);

    // Hide all pre-existing buttons.
    for(auto& button : mItemButtons)
    {
        button->SetEnabled(false);
    }

    // Need an inventory to populate.
    if(mCurrentInventory == nullptr)
    {
        return;
    }

    // Get active inventory item for actor.
    std::string activeInventoryItem = gInventoryManager.GetActiveInventoryItem(mCurrentActorName);

    // Populate the inventory screen.
    const float kStartX = 60.0f;
    const float kStartY = -90.0f;
    const float kSpacingX = 10.0f;
    const float kSpacingY = 10.0f;

    float x = kStartX;
    float y = kStartY;
    size_t counter = 0;
    for(auto& item : *mCurrentInventory)
    {
        // Only bother creating/positioning list item if we have a valid texture for it.
        //TODO: Use a placeholder/error texture?
        Texture* itemTexture = gInventoryManager.GetInventoryItemListTexture(item);
        if(itemTexture == nullptr)
        {
            gReportManager.Log("Error", "No inventory item texture found for " + item);
            continue;
        }

        // If this next item will go offscreen, we should move down to next row.
        if(x + itemTexture->GetWidth() > Window::GetWidth())
        {
            x = kStartX;
            y -= (itemTexture->GetHeight() + kSpacingY);
        }

        // Either reuse an already created button or create a new one.
        UIButton* button = nullptr;
        RectTransform* buttonRT = nullptr;
        if(counter < mItemButtons.size())
        {
            button = mItemButtons[counter];
            button->SetEnabled(true);

            buttonRT = button->GetOwner()->GetComponent<RectTransform>();
        }
        else
        {
            Actor* itemActor = new Actor(TransformType::RectTransform);
            itemActor->GetTransform()->SetParent(GetTransform());
            button = itemActor->AddComponent<UIButton>();

            buttonRT = itemActor->GetComponent<RectTransform>();
            buttonRT->SetAnchor(Vector2(0.0f, 1.0f));
            buttonRT->SetPivot(0.0f, 1.0f);

            mItemButtons.push_back(button);
        }
        ++counter;

        // Set position/size for button.
        buttonRT->SetAnchoredPosition(x, y);
        buttonRT->SetSizeDelta(itemTexture->GetWidth(), itemTexture->GetHeight());

        // Set texture for button.
        button->SetUpTexture(itemTexture);

        // Set button callback.
        button->SetPressCallback([this, item](UIButton* button) {
            OnItemClicked(button, item);
        });

        // See if this is the active inventory item.
        // If so, position the highlight over it.
        if(StringUtil::EqualsIgnoreCase(item, activeInventoryItem))
        {
            RectTransform* activeHighlightRT = mActiveHighlightImage->GetRectTransform();
            activeHighlightRT->SetAnchoredPosition(x + kActiveHighlightXOffset, y + 2); // nudge up slightly for better positioning
            mActiveHighlightImage->SetEnabled(true);
        }

        // Next button located to the right, with spacing.
        x += itemTexture->GetWidth() + kSpacingX;
    }
}

void InventoryScreen::OnUpdate(float deltaTime)
{
    // Escape key is a shortcut to exit. But be sure this screen's on the top of all others and no action is playing.
    if(gLayerManager.IsTopLayer(&mLayer) && !gActionManager.IsActionPlaying() && gInputManager.IsKeyLeadingEdge(SDL_SCANCODE_ESCAPE))
    {
        mExitButton->AnimatePress();
    }
}

void InventoryScreen::OnItemClicked(UIButton* button, std::string itemName)
{
	// Show the action bar for this noun.
    gActionManager.ShowActionBar(itemName, nullptr);

	// We want to add a "pickup" verb, which means to make the item the active inventory item.
    // This should go after the "LOOK" verb, but if there's no "LOOK" verb, it'll go at the beginning.
	ActionBar* actionBar = gActionManager.GetActionBar();
	actionBar->AddVerbAtIndex("PICKUP", actionBar->GetVerbIndex("LOOK") + 1, [this, itemName]() {
        gInventoryManager.SetActiveInventoryItem(this->mCurrentActorName, itemName);
	});

	// We want to also add an "inspect" verb, which means to show the close-up of the item.
    // This always goes at the front of the existing action bar.
	actionBar->AddVerbToFront("INSPECT", [itemName]() {
		gInventoryManager.InventoryInspect(itemName);
	});
}