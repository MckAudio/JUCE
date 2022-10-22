/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 7 End-User License
   Agreement and JUCE Privacy Policy.

   End User License Agreement: www.juce.com/juce-7-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

//==============================================================================
namespace XWindowSystemUtilities
{
    //==============================================================================
    /** A handy struct that uses XLockDisplay and XUnlockDisplay to lock the X server
        using RAII.

        @tags{GUI}
    */
    struct ScopedXLock
    {
        ScopedXLock();
        ~ScopedXLock();
    };

    //==============================================================================
    /** Gets a specified window property and stores its associated data, freeing it
        on deletion.

        @tags{GUI}
    */
    struct GetXProperty
    {
        GetXProperty (::Display* display, ::Window windowH, Atom property,
                      long offset, long length, bool shouldDelete, Atom requestedType);
        ~GetXProperty();

        bool success = false;
        unsigned char* data = nullptr;
        unsigned long numItems = 0, bytesLeft = 0;
        Atom actualType;
        int actualFormat = -1;
    };

    //==============================================================================
    /** Initialises and stores some atoms for the display.

        @tags{GUI}
    */
    struct Atoms
    {
        enum ProtocolItems
        {
            TAKE_FOCUS = 0,
            DELETE_WINDOW = 1,
            PING = 2
        };

        Atoms() = default;
        explicit Atoms (::Display*);

        static Atom getIfExists (::Display*, const char* name);
        static Atom getCreating (::Display*, const char* name);

        static String getName (::Display*, Atom);
        static bool isMimeTypeFile (::Display*, Atom);

        static constexpr unsigned long DndVersion = 3;

        Atom protocols, protocolList[3], changeState, state, userTime, activeWin, pid, windowType, windowState, windowStateHidden,
             XdndAware, XdndEnter, XdndLeave, XdndPosition, XdndStatus, XdndDrop, XdndFinished, XdndSelection,
             XdndTypeList, XdndActionList, XdndActionDescription, XdndActionCopy, XdndActionPrivate,
             XembedMsgType, XembedInfo, allowedActions[5], allowedMimeTypes[4], utf8String, clipboard, targets;
    };

    //==============================================================================
    /** Represents a setting according to the XSETTINGS specification.

        @tags{GUI}
    */
    struct XSetting
    {
        enum class Type
        {
            integer,
            string,
            colour,
            invalid
        };

        XSetting() = default;

        XSetting (const String& n, int v)            : name (n), type (Type::integer), integerValue (v)  {}
        XSetting (const String& n, const String& v)  : name (n), type (Type::string),  stringValue (v)   {}
        XSetting (const String& n, const Colour& v)  : name (n), type (Type::colour),  colourValue (v)   {}

        bool isValid() const noexcept  { return type != Type::invalid; }

        String name;
        Type type = Type::invalid;

        int integerValue = -1;
        String stringValue;
        Colour colourValue;
    };

    /** Parses and stores the X11 settings for a display according to the XSETTINGS
        specification.

        @tags{GUI}
    */
    class XSettings
    {
    public:
        static std::unique_ptr<XSettings> createXSettings (::Display*);

        //==============================================================================
        void update();
        ::Window getSettingsWindow() const noexcept  { return settingsWindow; }

        XSetting getSetting (const String& settingName) const;

        //==============================================================================
        struct Listener
        {
            virtual ~Listener() = default;
            virtual void settingChanged (const XSetting& settingThatHasChanged) = 0;
        };

        void addListener (Listener* listenerToAdd)        { listeners.add (listenerToAdd); }
        void removeListener (Listener* listenerToRemove)  { listeners.remove (listenerToRemove); }

    private:
        ::Display* display = nullptr;
        ::Window settingsWindow = None;
        Atom settingsAtom;

        int lastUpdateSerial = -1;

        std::unordered_map<String, XSetting> settings;
        ListenerList<Listener> listeners;

        XSettings (::Display*, Atom, ::Window);

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XSettings)
    };
}

//==============================================================================
class LinuxComponentPeer;

class XWindowSystem  : public DeletedAtShutdown
{
public:
    //==============================================================================
    ::Window createWindow (::Window parentWindow, LinuxComponentPeer*) const;
    void destroyWindow    (::Window);

    void setTitle (::Window, const String&) const;
    void setIcon (::Window , const Image&) const;
    void setVisible (::Window, bool shouldBeVisible) const;
    void setBounds (::Window, Rectangle<int>, bool fullScreen) const;
    void updateConstraints (::Window) const;

    ComponentPeer::OptionalBorderSize getBorderSize (::Window) const;
    Rectangle<int> getWindowBounds (::Window, ::Window parentWindow);
    Point<int> getPhysicalParentScreenPosition() const;

    bool contains (::Window, Point<int> localPos) const;

    void setMinimised (::Window, bool shouldBeMinimised) const;
    bool isMinimised  (::Window) const;

    void setMaximised (::Window, bool shouldBeMinimised) const;

    void toFront  (::Window, bool makeActive) const;
    void toBehind (::Window, ::Window otherWindow) const;

    bool isFocused (::Window) const;
    bool grabFocus (::Window) const;

    bool canUseSemiTransparentWindows() const;
    bool canUseARGBImages() const;
    bool isDarkModeActive() const;

    int getNumPaintsPendingForWindow (::Window);
    void processPendingPaintsForWindow (::Window);
    void addPendingPaintForWindow (::Window);
    void removePendingPaintForWindow (::Window);

    Image createImage (bool isSemiTransparentWindow, int width, int height, bool argb) const;
    void blitToWindow (::Window, Image, Rectangle<int> destinationRect, Rectangle<int> totalRect) const;

    void setScreenSaverEnabled (bool enabled) const;

    Point<float> getCurrentMousePosition() const;
    void setMousePosition (Point<float> pos) const;

    Cursor createCustomMouseCursorInfo (const Image&, Point<int> hotspot) const;
    void deleteMouseCursor (Cursor cursorHandle) const;
    Cursor createStandardMouseCursor (MouseCursor::StandardCursorType) const;
    void showCursor (::Window, Cursor cursorHandle) const;

    bool isKeyCurrentlyDown (int keyCode) const;
    ModifierKeys getNativeRealtimeModifiers() const;

    Array<Displays::Display> findDisplays (float masterScale) const;

    ::Window createKeyProxy (::Window) const;
    void deleteKeyProxy (::Window) const;

    bool externalDragFileInit (LinuxComponentPeer*, const StringArray& files, bool canMove, std::function<void()>&& callback) const;
    bool externalDragTextInit (LinuxComponentPeer*, const String& text, std::function<void()>&& callback) const;

    void copyTextToClipboard (const String&);
    String getTextFromClipboard() const;
    String getLocalClipboardContent() const noexcept  { return localClipboardContent; }

    ::Display* getDisplay() const noexcept                            { return display; }
    const XWindowSystemUtilities::Atoms& getAtoms() const noexcept    { return atoms; }
    XWindowSystemUtilities::XSettings* getXSettings() const noexcept  { return xSettings.get(); }

    bool isX11Available() const noexcept  { return xIsAvailable; }

    static String getWindowScalingFactorSettingName()  { return "Gdk/WindowScalingFactor"; }
    static String getThemeNameSettingName()            { return "Net/ThemeName"; }

    //==============================================================================
    void handleWindowMessage (LinuxComponentPeer*, XEvent&) const;
    bool isParentWindowOf (::Window, ::Window possibleChild) const;

    //==============================================================================
    JUCE_DECLARE_SINGLETON (XWindowSystem, false)

private:
    XWindowSystem();
    ~XWindowSystem();

    //==============================================================================
    struct VisualAndDepth
    {
        Visual* visual;
        int depth;
    };

    struct DisplayVisuals
    {
        explicit DisplayVisuals (::Display*);

        VisualAndDepth getBestVisualForWindow (bool) const;
        bool isValid() const noexcept;

        Visual* visual16Bit = nullptr;
        Visual* visual24Bit = nullptr;
        Visual* visual32Bit = nullptr;
    };

    bool initialiseXDisplay();
    void destroyXDisplay();

    //==============================================================================
    ::Window getFocusWindow (::Window) const;

    bool isFrontWindow (::Window) const;

    //==============================================================================
    void xchangeProperty (::Window, Atom, Atom, int, const void*, int) const;

    void removeWindowDecorations (::Window) const;
    void addWindowButtons        (::Window, int) const;
    void setWindowType           (::Window, int) const;

    void initialisePointerMap();
    void deleteIconPixmaps (::Window) const;
    void updateModifierMappings() const;

    long getUserTime (::Window) const;

    void initialiseXSettings();

    int setupXTouch(::Window) const;

    //==============================================================================
    void handleKeyPressEvent        (LinuxComponentPeer*, XKeyEvent&) const;
    void handleKeyReleaseEvent      (LinuxComponentPeer*, const XKeyEvent&) const;
    void handleWheelEvent           (LinuxComponentPeer*, const XButtonPressedEvent&, float) const;
    void handleButtonPressEvent     (LinuxComponentPeer*, const XButtonPressedEvent&, int) const;
    void handleButtonPressEvent     (LinuxComponentPeer*, const XButtonPressedEvent&) const;
    void handleButtonReleaseEvent   (LinuxComponentPeer*, const XButtonReleasedEvent&) const;
    void handleMotionNotifyEvent    (LinuxComponentPeer*, const XPointerMovedEvent&) const;
    void handleEnterNotifyEvent     (LinuxComponentPeer*, const XEnterWindowEvent&) const;
    void handleLeaveNotifyEvent     (LinuxComponentPeer*, const XLeaveWindowEvent&) const;
    void handleFocusInEvent         (LinuxComponentPeer*) const;
    void handleFocusOutEvent        (LinuxComponentPeer*) const;
    void handleExposeEvent          (LinuxComponentPeer*, XExposeEvent&) const;
    void handleConfigureNotifyEvent (LinuxComponentPeer*, XConfigureEvent&) const;
    void handleGravityNotify        (LinuxComponentPeer*) const;
    void propertyNotifyEvent        (LinuxComponentPeer*, const XPropertyEvent&) const;
    void handleMappingNotify        (XMappingEvent&) const;
    void handleClientMessageEvent   (LinuxComponentPeer*, XClientMessageEvent&, XEvent&) const;
    void handleXEmbedMessage        (LinuxComponentPeer*, XClientMessageEvent&) const;

    void handleGenericEvent         (LinuxComponentPeer*, const XIDeviceEvent*) const;
    void handleTouchPressEvent      (LinuxComponentPeer*, const XIDeviceEvent*);
    void handleTouchReleaseEvent    (LinuxComponentPeer*, const XIDeviceEvent*);
    void handleTouchUpdateEvent     (LinuxComponentPeer*, const XIDeviceEvent*);

    void dismissBlockingModals      (LinuxComponentPeer*) const;
    void dismissBlockingModals      (LinuxComponentPeer*, const XConfigureEvent&) const;
    void updateConstraints          (::Window, ComponentPeer&) const;

    ::Window findTopLevelWindowOf (::Window) const;

    static void windowMessageReceive (XEvent&);

    //==============================================================================
    bool xIsAvailable = false;

    XWindowSystemUtilities::Atoms atoms;
    ::Display* display = nullptr;
    std::unique_ptr<DisplayVisuals> displayVisuals;
    std::unique_ptr<XWindowSystemUtilities::XSettings> xSettings;

   #if JUCE_USE_XSHM
    std::map<::Window, int> shmPaintsPendingMap;
   #endif

   #if JUCE_USE_XINPUT2
    int xi2opcode;
   #endif
    std::map<int, int> xiTouchIds;
    std::set<int> xiFreeTouchIds;
    int currentTouchIdx = 0;

    int shmCompletionEvent = 0;
    int pointerMap[5] = {};
    String localClipboardContent;

    Point<int> parentScreenPosition;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XWindowSystem)
};

} // namespace juce
