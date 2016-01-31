#ifndef OSEVENT_H
#define OSEVENT_H

enum OsKeyCode {
    OSKEY_LEFT_ARROW,
    OSKEY_RIGHT_ARROW,
    OSKEY_UP_ARROW,
    OSKEY_DOWN_ARROW,
    OSKEY_DELETE,
    OSKEY_ESCAPE,
    OSKEY_ENTER,
    OSKEY_SPACE,
    OSKEY_LBRACKET,
    OSKEY_RBRACKET,
    OSKEY_SLASH,
    OSKEY_PERIOD,
    OSKEY_COMMA,
    OSKEY_QUOTE,
    OSKEY_EQUAL,
    OSKEY_SEMICOLON,
    OSKEY_MINUS,
    OSKEY_NUM0,
    OSKEY_NUM1,
    OSKEY_NUM2,
    OSKEY_NUM3,
    OSKEY_NUM4,
    OSKEY_NUM5,
    OSKEY_NUM6,
    OSKEY_NUM7,
    OSKEY_NUM8,
    OSKEY_NUM9,
    OSKEY_A,
    OSKEY_B,
    OSKEY_C,
    OSKEY_D,
    OSKEY_E,
    OSKEY_F,
    OSKEY_G,
    OSKEY_H,
    OSKEY_I,
    OSKEY_J,
    OSKEY_K,
    OSKEY_L,
    OSKEY_M,
    OSKEY_N,
    OSKEY_O,
    OSKEY_P,
    OSKEY_Q,
    OSKEY_R,
    OSKEY_S,
    OSKEY_T,
    OSKEY_U,
    OSKEY_V,
    OSKEY_W,
    OSKEY_X,
    OSKEY_Y,
    OSKEY_Z,
    OSKEY_LAST,
};

enum OsMouseButton {
    OSMOUSE_BUTTON_LEFT,
    OSMOUSE_BUTTON_RIGHT,
};

enum OsEventType {
    OSEVENT_IDLE,
    OSEVENT_PAINT,
    OSEVENT_WINDOW_RESIZE,
    OSEVENT_KEY_DOWN,
    OSEVENT_KEY_UP,
    OSEVENT_KEY_DOWN_REPEAT,
    OSEVENT_CHAR,
    OSEVENT_MOUSE_BUTTON_DOWN,
    OSEVENT_MOUSE_BUTTON_UP,
    OSEVENT_MOUSE_DRAG,
    OSEVENT_MOUSE_WHEEL,
};

struct OsKeyEvent {
    enum {
        FLAG_ALT = 1,
        FLAG_CONTROL = 2,
        FLAG_SHIFT = 4,
    };

    OsKeyCode code;
    unsigned modifierFlags;
};

struct OsCharEvent {
    unsigned int ch;
};

struct OsMouseButtonEvent {
    OsMouseButton button;
    int x;
    int y;
    float normalizedX;
    float normalizedY;
};

struct OsMouseDragEvent {
    OsMouseButton button;
    int x;
    int y;
    float normalizedX;
    float normalizedY;
    int deltaX;
    int deltaY;
    float normalizedDeltaX;
    float normalizedDeltaY;
};

struct OsMouseWheelEvent {
    float amount;
    int x;
    int y;
};

struct OsEvent {
    OsEventType type;
    union {
        OsKeyEvent key;
        OsCharEvent character;
        OsMouseButtonEvent mouseButton;
        OsMouseDragEvent mouseDrag;
        OsMouseWheelEvent mouseWheel;
    };
};

#endif // OSEVENT_H
