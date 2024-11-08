## Example Codes

Here are some example of how to use the OFM-DeviceDisplay library:
In Main.Cpp:
```cpp
// This are the custom widgets for the display, which can be used in the own application modules


#ifdef DEVICE_DISPLAY_MODULE
    #include "DeviceDisplay.h"
#endif

#ifdef DEVICE_DISPLAY_MODULE
// Example console conversation lines
const char* conversationLines[] = {
    "1",
    "12",
    "123"
    "1234",
    "12345",
    "123456",
    "1234567",
    "12345678",
    "123456789",
    "1234567890",
    "12345678901",
    "123456789012",
    "1234567890123",
    "12345678901234",
    "123456789012345"};
const int numLines = sizeof(conversationLines) / sizeof(conversationLines[0]);
int currentLineIndex = 0;
uint32_t lastUpdateTime = 0;
uint32_t lastUpdateTime2 = 0;

void demoWidgetSetup()
{
    // This are the custom widgets for the display, which can be used in the own application modules
    // Example: openknxDisplayModule.addWidget(&textWidget, 5000); // Show TextWidget for 5 seconds

    // Example Widget: Show the OpenKNX System Information
    Widget* WidgetSysInfo = new Widget(Widget::DisplayMode::DYNAMIC_TEXT);                                 // Initialize the display and widgets
    WidgetSysInfo->textLines[0].textSize = 2;                                                              // Set the text size for the header
    WidgetSysInfo->SetDynamicTextLines(                                                                    // Set the text lines and the information to display
        {"OpenKNX",                                                                                        // Header
         "",                                                                                               // Line 1
         String("Dev.: " + String(MAIN_OrderNumber)).c_str(),                                              // Line 2
         String("Ver.: " + String(openknx.info.humanFirmwareVersion().c_str())).c_str(),                   // Line 3
         String("Addr.: " + String(openknx.info.humanIndividualAddress().c_str())).c_str(),                // Line 4
         String("Free mem: " + String((float)freeMemory() / 1024) + " KiB").c_str(),                       // Line 5
         String("    (min. " + String((float)openknx.common.freeMemoryMin() / 1024) + " KiB)").c_str()});  // Line 6
    openknxDisplayModule.addWidget(WidgetSysInfo, 5000, "SysInfo", DeviceDisplay::WidgetAction::NoAction); // Add the widget to the display queue.

    //
    //  Example Widget: Show the Network Information
    //
    Widget* WidgetNetInfo = new Widget(Widget::DisplayMode::DYNAMIC_TEXT);
    {
        WidgetNetInfo->textLines[0].textSize = 1;                            // Set the text size for the header
        WidgetNetInfo->textLines[1].textSize = 1;                            // Set the text size for the Text line 1
        WidgetNetInfo->textLines[2].textSize = 1;                            // Set the text size for the Text line 2
        WidgetNetInfo->textLines[3].textSize = 1;                            // Set the text size for the Text line 3
        WidgetNetInfo->textLines[4].textSize = 1;                            // Set the text size for the Text line 4
        WidgetNetInfo->textLines[0].alignPos = TextDynamicAlign::ALIGN_LEFT; // Set the alignment for the header
        WidgetNetInfo->textLines[1].alignPos = TextDynamicAlign::ALIGN_LEFT; // Set the alignment for the Text line 1
        WidgetNetInfo->textLines[2].alignPos = TextDynamicAlign::ALIGN_LEFT; // Set the alignment for the Text line 2
        WidgetNetInfo->textLines[3].alignPos = TextDynamicAlign::ALIGN_LEFT; // Set the alignment for the Text line 3
        WidgetNetInfo->textLines[4].alignPos = TextDynamicAlign::ALIGN_LEFT; // Set the alignment for the Text line 4
        WidgetNetInfo->SetDynamicTextLines(                                  // Set the text lines and the information to display
            {
                "Network:",              // Header
                "IP: 11.11.0.123",       // Line 1 i.e.: String("IP: " + String(currentIpAddress.c_str())).c_str(),
                "Subnet: 255.255.255.0", // Line 2
                "Gateway: 11.11.0.1",    // Line 3
                "DNS: 11.11.0.1"         // Line 4
            });
    }
    openknxDisplayModule.addWidget(WidgetNetInfo, 3000, "NetInfo"); // Add the widget to the display queue.
                                                                    // Defaul Action: Regular Widget, wich is the action NoAction!

    // Example Widget: Show the OpenKNX Logo
    // Since the OpenKNX logo is a part of the display module, it is not necessary to add it to the queue.
    Widget* WidgetOKNXlogo = new Widget(Widget::DisplayMode::OPENKNX_LOGO);
    openknxDisplayModule.addWidget(WidgetOKNXlogo, 3000, "UptimeLogo");

    // Example Widget: Text Widget with scrolling text
    Widget* dynTextWidgetFull = new Widget(Widget::DisplayMode::DYNAMIC_TEXT);
    dynTextWidgetFull->SetDynamicTextLines(
        {
            "H: Fixed!",                       // Test Line 1
            "L2: This is a scroll test Line2", // Test Line 2
            "L3: This is Line3",               // Test Line 3
            "L4: This is a scroll test Line4", // Test Line 4
            "L5: This is Line5",               // Test Line 5
            "L6: This is a scroll test Line6", // Test Line 6
            "L7: This is Line7",               // Test Line 7
            "L8: This is a scroll test Line8"  // Test Line 8
        });
    openknxDisplayModule.addWidget(dynTextWidgetFull, 3000, "DynamicText");

    // Example Widget: Header at the top, centered and middle-aligned line below
    Widget* dynTextWidget_Header_and_1_Line = new Widget(Widget::DisplayMode::DYNAMIC_TEXT);                                  // Create a new dynamic text widget
    dynTextWidget_Header_and_1_Line->textLines[1].textSize = 4;                                                               // Set the text size for the line below the header
    dynTextWidget_Header_and_1_Line->textLines[0].alignPos = TextDynamicAlign::ALIGN_CENTER | TextDynamicAlign::ALIGN_TOP;    // Set the alignment for the header
    dynTextWidget_Header_and_1_Line->textLines[1].alignPos = TextDynamicAlign::ALIGN_CENTER | TextDynamicAlign::ALIGN_MIDDLE; // Set the alignment for the line below the header
    dynTextWidget_Header_and_1_Line->SetDynamicTextLine(0, "H: Centered");                                                    // Set the header text
    dynTextWidget_Header_and_1_Line->SetDynamicTextLine(1, "L1: Middle Centered");                                            // Set the line below the header
    openknxDisplayModule.addWidget(dynTextWidget_Header_and_1_Line, 3000, "DynamicText_Header_and_1_Line");

    // Example Widget:: Header aligned to the left, Footer aligned to the right
    Widget* dynTextWidget_LeftHeader_RightFooter = new Widget(Widget::DisplayMode::DYNAMIC_TEXT);                                 // Create a new dynamic text widget
    dynTextWidget_LeftHeader_RightFooter->textLines[0].textSize = 1;                                                              // Header text size
    dynTextWidget_LeftHeader_RightFooter->textLines[1].textSize = 1;                                                              // Footer text size
    dynTextWidget_LeftHeader_RightFooter->textLines[0].alignPos = TextDynamicAlign::ALIGN_LEFT | TextDynamicAlign::ALIGN_TOP;     // Header alignment
    dynTextWidget_LeftHeader_RightFooter->textLines[1].alignPos = TextDynamicAlign::ALIGN_RIGHT | TextDynamicAlign::ALIGN_BOTTOM; // Footer alignment
    dynTextWidget_LeftHeader_RightFooter->SetDynamicTextLines(
        {
            "H:Left Aligned", // Expected top-left of the display
            "F:Right Aligned" // Expected bottom-right of the display
        });
    openknxDisplayModule.addWidget(dynTextWidget_LeftHeader_RightFooter, 3000, "DynamicText_LeftHeader_RightFooter");

    // Example Widget: Three lines - Top, Center, Bottom alignment (all centered horizontally)
    Widget* dynTextWidget_TopCenterBottom = new Widget(Widget::DisplayMode::DYNAMIC_TEXT); // Create a new dynamic text widget
    dynTextWidget_TopCenterBottom->textLines[0].textSize = 1;                              // Set the text size for the header. Default is 1
    dynTextWidget_TopCenterBottom->textLines[0].alignPos = TextDynamicAlign::ALIGN_CENTER | TextDynamicAlign::ALIGN_TOP;
    dynTextWidget_TopCenterBottom->textLines[1].alignPos = TextDynamicAlign::ALIGN_CENTER | TextDynamicAlign::ALIGN_MIDDLE;
    dynTextWidget_TopCenterBottom->textLines[2].alignPos = TextDynamicAlign::ALIGN_CENTER | TextDynamicAlign::ALIGN_BOTTOM;
    dynTextWidget_TopCenterBottom->SetDynamicTextLines( // Set the text lines and the information to display
        {
            "Top Centered",    // Expected top-center
            "Middle Centered", // Expected center of the display
            "Bottom Centered"  // Expected bottom-center
        });
    openknxDisplayModule.addWidget(dynTextWidget_TopCenterBottom, 3000, "DynamicText_TopCenterBottom");

    // Example Widget: Multiple lines with stacked positioning (default without specific alignment flags)
    Widget* dynTextWidget_DefaultStacking = new Widget(Widget::DisplayMode::DYNAMIC_TEXT); // Create a new dynamic text widget
    dynTextWidget_DefaultStacking->textLines[1].textSize = 2;                              // Larger text for line 1
    dynTextWidget_DefaultStacking->textLines[2].textSize = 2;                              // Larger text for line 2
    dynTextWidget_DefaultStacking->SetDynamicTextLines(                                    // Set the text lines and the information to display
        {
            "L1: Top",    // Expected at the very top
            "L2: Line 1", // Below line 1
            "L3: Line 2"  // Below line 2, all stacked from the top
        });
    openknxDisplayModule.addWidget(dynTextWidget_DefaultStacking, 3000, "DynamicText_DefaultStacking");

    // Example Widget: Center and middle alignment with scrollable text
    Widget* dynTextWidget_ScrollingCentered = new Widget(Widget::DisplayMode::DYNAMIC_TEXT);
    dynTextWidget_ScrollingCentered->textLines[0].alignPos = TextDynamicAlign::ALIGN_CENTER | TextDynamicAlign::ALIGN_TOP;
    dynTextWidget_ScrollingCentered->textLines[1].alignPos = TextDynamicAlign::ALIGN_CENTER | TextDynamicAlign::ALIGN_MIDDLE;
    dynTextWidget_ScrollingCentered->textLines[4].scrollText = false; // Do not scroll the Line 4! Default is True!
    dynTextWidget_ScrollingCentered->SetDynamicTextLines(
        {
            "H: Top Cententer",                                            // Centered at the top
            "",                                                            // Line 1 is empty
            "",                                                            // Line 2 is empty
            "",                                                            // Line 3 is empty
            "Not Scrollable. To long text for the display, do not scroll", // Centered in the middle, does not scroll
            "Scrollable: This line will scroll if too long."               // Centered in the middle, scrolls if it exceeds width
        });
    openknxDisplayModule.addWidget(dynTextWidget_ScrollingCentered, 3000, "DynamicText_ScrollingCentered");

    // Example Widget: Center and middle alignment with scrollable text
    Widget* dynTextWidget_ScrollingCentered_skipLines = new Widget(Widget::DisplayMode::DYNAMIC_TEXT);
    dynTextWidget_ScrollingCentered_skipLines->textLines[0].alignPos = TextDynamicAlign::ALIGN_CENTER | TextDynamicAlign::ALIGN_TOP;
    dynTextWidget_ScrollingCentered_skipLines->textLines[7].alignPos = TextDynamicAlign::ALIGN_LEFT; // Align the last line to the left
    dynTextWidget_ScrollingCentered_skipLines->textLines[1].skipLineIfEmpty = true;                  // Skip empty line 1
    dynTextWidget_ScrollingCentered_skipLines->textLines[2].skipLineIfEmpty = true;                  // Skip empty line 2
    dynTextWidget_ScrollingCentered_skipLines->textLines[3].skipLineIfEmpty = true;                  // Skip empty line 3
    dynTextWidget_ScrollingCentered_skipLines->textLines[4].skipLineIfEmpty = true;                  // Skip empty line 4
    dynTextWidget_ScrollingCentered_skipLines->textLines[5].skipLineIfEmpty = true;                  // Skip empty line 5
    dynTextWidget_ScrollingCentered_skipLines->textLines[6].skipLineIfEmpty = true;                  // Skip empty line 6
    dynTextWidget_ScrollingCentered_skipLines->SetDynamicTextLines(
        {
            "H: Top Cententer",                                              // Centered at the top
            "", "", "", "", "", "",                                          // Empty lines. Those will be skipped, since the flag is set to skip empty lines
            "NO SKIP LINES - Scrollable: This line will scroll if too long." // Centered in the middle, scrolls if it exceeds width
            // This is line 8, since the empty lines are skipped, this line will be displayed in place of the Line 1 !
        });
    openknxDisplayModule.addWidget(dynTextWidget_ScrollingCentered_skipLines, 3000, "DynamicText_ScrollingCentered_skipLines");

    // Example Widget: Left-aligned text at the top and right-aligned text in the middle
    Widget* dynTextWidget_LeftTop_RightMiddle = new Widget(Widget::DisplayMode::DYNAMIC_TEXT);
    dynTextWidget_LeftTop_RightMiddle->textLines[0].textSize = 2; // Larger text for header
    dynTextWidget_LeftTop_RightMiddle->textLines[0].alignPos = TextDynamicAlign::ALIGN_LEFT | TextDynamicAlign::ALIGN_TOP;
    dynTextWidget_LeftTop_RightMiddle->textLines[1].alignPos = TextDynamicAlign::ALIGN_RIGHT | TextDynamicAlign::ALIGN_MIDDLE;
    dynTextWidget_LeftTop_RightMiddle->SetDynamicTextLine(0, "H: Left Aligned");         // Set the header text
    dynTextWidget_LeftTop_RightMiddle->SetDynamicTextLine(1, "Right Aligned in Middle"); // Set the line below the header
    openknxDisplayModule.addWidget(dynTextWidget_LeftTop_RightMiddle, 3000, "DynamicText_LeftTop_RightMiddle");

    // Example Widget: Print all possible characters on the display
    Widget* dynTextWidget_AllChars = new Widget(Widget::DisplayMode::DYNAMIC_TEXT);
    dynTextWidget_AllChars->textLines[0].textSize = 1; // Set the text size for the header
    dynTextWidget_AllChars->SetDynamicTextLines(       // Set the text lines and the information to display
        {
            "All Characters:",                   // Header
            "0123456789",                        // Line 1
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ",        // Line 2
            "abcdefghijklmnopqrstuvwxyz",        // Line 3
            "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~" // Line 4
        });
    openknxDisplayModule.addWidget(dynTextWidget_AllChars, 3000, "DynamicText_AllChars");
    #ifdef QRCODE_WIDGET
    // Example Widget: Show a QR code
    Widget* QRCodeWidget = new Widget(Widget::DisplayMode::QR_CODE);                 // Create a new QR code widget
    QRCodeWidget->qrCodeWidget.setUrl("https://www.openknx.de");                     // Set the URL for the QR code
    QRCodeWidget->qrCodeWidget.setAlign(QRCodeWidget::QRCodeAlignPos::ALIGN_CENTER); // Set the alignment for the QR code
    openknxDisplayModule.addWidget(QRCodeWidget, 15000, "QRCode",
                                   DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget. The status flag will be displayed immediately
                                       DeviceDisplay::WidgetAction::InternalEnabled | // This widget is enabled
                                       DeviceDisplay::WidgetAction::AutoRemoveFlag);  // Remove this widget after display of the set duration time. Here 10sec.
    #endif

    // Example Widget: Console Widget. This widget is used to display a console simulatted output.
    Widget* myConsoleWidget = new Widget(Widget::DisplayMode::DYNAMIC_TEXT);
    myConsoleWidget->setAllowEmptyTextLines(true); // Allow empty text lines
    openknxDisplayModule.addWidget(myConsoleWidget, 30000, "consoleWidget");
}

void demoWidgetLoop()
{
    if (delayCheck(lastUpdateTime2, 1000) && openknxDisplayModule.isWidgetCurrentlyDisplayed("SysInfo")) // Update the display every second!
    {
        Widget* sysInfoWidget = openknxDisplayModule.getWidgetInfo("SysInfo")->widget;
        // Do the update only if the widget is currently displayed

        if (sysInfoWidget != nullptr)
        {
            sysInfoWidget->SetDynamicTextLine(1, String("Uptime: " + String(openknx.logger.buildUptime().c_str())).c_str());
            sysInfoWidget->SetDynamicTextLine(4, String("Addr.: " + String(openknx.info.humanIndividualAddress().c_str())).c_str());
            sysInfoWidget->SetDynamicTextLine(5, String("Free mem: " + String((float)freeMemory() / 1024) + " KiB").c_str());
            sysInfoWidget->SetDynamicTextLine(6, String("    (min. " + String((float)openknx.common.freeMemoryMin() / 1024) + " KiB)").c_str());
        }
    }
    if (delayCheck(lastUpdateTime, 1000) && openknxDisplayModule.isWidgetCurrentlyDisplayed("consoleWidget")) // Update the display every second!
    {
        if (currentLineIndex < numLines)
        {
            DeviceDisplay::WidgetInfo* consoleWidgetInfo = openknxDisplayModule.getWidgetInfo("consoleWidget");
            Widget* consoleWidget = consoleWidgetInfo->widget;
            if (consoleWidget != nullptr)
            {
                consoleWidget->appendLine(conversationLines[currentLineIndex]);
                currentLineIndex++;
            }
        }
        else
        {
            currentLineIndex = 0;
        }
        lastUpdateTime = millis();
    }
}
#endif

// Your main.cpp setup
void setup()
{
    const uint8_t firmwareRevision = 6;
    openknx.init(firmwareRevision);
    openknx.addModule(1, openknxLogic);
#ifdef DEVICE_DISPLAY_MODULE
    openknx.addModule(2, openknxDisplayModule);
#endif

    openknx.setup();
#ifdef DEVICE_DISPLAY_MODULE
    demoWidgetSetup();
#endif

// Your main.cpp Loop
void loop()
{
    openknx.loop();
    void demoWidgetLoop();
}
```