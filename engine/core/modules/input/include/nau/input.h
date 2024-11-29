// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include "nau/utils/enum/enum_reflection.h"

namespace nau::input
{

  /**
   * @brief Determines all supported keyboard inputs.
   */
  NAU_DEFINE_ENUM_(Key,
  
    Escape,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    F13,
    F14,
    F15,
    F16,
    F17,
    F18,
    F19,
    Print,
    ScrollLock,
    Break,

    Space,

    Apostrophe,
    Comma,
    Minus,
    Period,
    Slash,

    N0,
    N1,
    N2,
    N3,
    N4,
    N5,
    N6,
    N7,
    N8,
    N9,

    Semicolon,
    Less,
    Equal,

    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    BracketLeft,
    Backslash,
    BracketRight,

    Grave,

    Left,
    Right,
    Up,
    Down,
    Insert,
    Home,
    Delete,
    End,
    PageUp,
    PageDown,

    NumLock,
    KpEqual,
    KpDivide,
    KpMultiply,
    KpSubtract,
    KpAdd,
    KpEnter,
    KpInsert, // 0
    KpEnd, // 1
    KpDown, // 2
    KpPageDown, // 3
    KpLeft, // 4
    KpBegin, // 5
    KpRight, // 6
    KpHome, // 7
    KpUp, // 8
    KpPageUp, // 9
    KpDelete, // ,

    BackSpace,
    Tab,
    Return,
    CapsLock,
    ShiftL,
    CtrlL,
    SuperL,
    AltL,
    AltR,
    SuperR,
    Menu,
    CtrlR,
    ShiftR,

    Back,
    SoftLeft,
    SoftRight,
    Call,
    Endcall,
    Star,
    Pound,
    DpadCenter,
    VolumeUp,
    VolumeDown,
    Power,
    Camera,
    Clear,
    Symbol,
    Explorer,
    Envelope,
    Equals,
    At,
    Headsethook,
    Focus,
    Plus,
    Notification,
    Search,
    MediaPlayPause,
    MediaStop,
    MediaNext,
    MediaPrevious,
    MediaRewind,
    MediaFastForward,
    Mute,
    Pictsymbols,
    SwitchCharset,

    Forward,
    Extra1,
    Extra2,
    Extra3,
    Extra4,
    Extra5,
    Extra6,
    Fn,

    Circumflex,
    Ssharp,
    Acute,
    AltGr,
    Numbersign,
    Udiaeresis,
    Adiaeresis,
    Odiaeresis,
    Section,
    Aring,
    Diaeresis,
    Twosuperior,
    RightParenthesis,
    Dollar,
    Ugrave,
    Asterisk,
    Colon,
    Exclam,

    BraceLeft,
    BraceRight,
    SysRq,
    KeyCount_ /** < Determines the total number of the supported keyboard inputs. */
  );

  /**
   * @brief Determines all supported keyboard inputs.
   */
  NAU_DEFINE_ENUM_(MouseKey,
  
    Button0 = 0,
    ButtonLeft = Button0,
    Button1,
    ButtonMiddle = Button1,
    Button2,
    ButtonRight = Button2,
    Button3,
    Button4,
    Button5,
    Button6,
    Button7,
    Button8,
    Button9,
    Button10,
    Button11,
    Button12,
    Button13,
    Button14,
    Button15,
    Button16,
    Button17,
    Button18,
    Button19,
    Button20,
    ButtonMax = Button20,               /** < Determines the maximal mouse button index.*/
    ButtonCount,                        /** < Determines the total number of supported mouse buttons.*/
    AxisX = ButtonCount,                /** < Determines mouse cursor X-coordinate. */
    AxisY,                              /** < Determines mouse cursor Y-coordinate. */
    Wheel,                              /** < Determines mouse wheel */
    HWheel,                             /** < Determines mouse hwheel */
    ButtonCount_,                       /** < Determines the total number of supported mouse inputs.*/
    AxisCount = ButtonCount_ - AxisX    /** < Determines the total number of supported mouse axes. */
  );


  /**
   * @brief Informs the input manager of the screen resolution.
   * 
   * @param [in] x, y   Screen resolution.
   * 
   * It is necessary for the manager to be aware of the screen size for correct mouse input mapping.
   */
  NAU_COREINPUT_EXPORT void setScreenResolution(int x, int y);

  /** 
   * @brief Updates the input state.
   * 
   * If the input manager implementation does not use system time (i.e. `useSystemTime` is set to `false`), prefer update(float dt) over this function.
   */
  NAU_COREINPUT_EXPORT void update();

  /** 
   * @brief Updates the input state.
   * 
   * @param [in] dt Delta time.
   * 
   * If the input manager implementation uses system time (i.e. `useSystemTime` is set to `true`), prefer update() over this function.
   */
  NAU_COREINPUT_EXPORT void update(float dt);

  // Keyboard functions

  /**
   * @brief Retrieves the number of currently registered keyboard devices.
   * 
   * @return Number of keyboard devices.
   */
  NAU_COREINPUT_EXPORT int getKeyboardDeviceCount();

  /**
   * @brief Checks whether the keyboard button has just been pressed.
   * 
   * @param [in] deviceId   Index of the device to check.
   * @param [in] key        Code of the button to check.
   * @return                `true` if the button has changed its state from 'Pressed' to 'Released' within this frame, `false` otherwise.
   */
  NAU_COREINPUT_EXPORT bool isKeyboardButtonPressed(int deviceId, nau::input::Key key);

  /**
   * @brief Checks whether the keyboard button is being held down currently.
   *
   * @param [in] deviceId   Index of the device to check.
   * @param [in] key        Code of the button to check.
   * @return                `true` if the button is in 'Pressed' state, `false` otherwise.
   */
  NAU_COREINPUT_EXPORT bool isKeyboardButtonHold(int deviceId, nau::input::Key key);


  // Mouse functions

  /**
   * @brief Retrieves the number of currently registered mouse devices.
   *
   * @return Number of mouse devices.
   */
  NAU_COREINPUT_EXPORT int getMouseDeviceCount();

  /**
   * @brief Checks if the mouse button has just been pressed.
   * 
   * @param [in] deviceId   Index of the device to check.
   * @param [in] key        Code of the button to check.
   * @return                `true` if the button has changed its state from 'Pressed' to 'Released' within this frame, `false` otherwise.
   */
  NAU_COREINPUT_EXPORT bool isMouseButtonPressed(int deviceId, nau::input::MouseKey key);

  /**
   * @brief Checks if the mouse button has just been released.
   *
   * @param [in] deviceId   Index of the device to check.
   * @param [in] key        Code of the button to check.
   * @return                `true` if the button has changed its state from 'Released' to 'Pressed' within this frame, `false` otherwise.
   */
  NAU_COREINPUT_EXPORT bool isMouseButtonReleased(int deviceId, nau::input::MouseKey key);

  /**
   * @brief Checks whether the mouse button is being held down currently.
   *
   * @param [in] deviceId   Index of the device to check.
   * @param [in] key        Code of the button to check.
   * @return                `true` if the button is in 'Pressed' state, `false` otherwise.
   */
  NAU_COREINPUT_EXPORT bool isMouseButtonHold(int deviceId, nau::input::MouseKey key);

  /**
   * @brief Retrieves the mouse axis absolute value.
   * 
   * @param [in] deviceId   Index of the device to check.
   * @param [in] key        Code of the axis to retrieve. It has to be either nau::input::MouseKey::AxisX or nau::input::MouseKey::AxisY.
   * @return                Absolute axis value.
   */
  NAU_COREINPUT_EXPORT float getMouseAxisValue(int deviceId, nau::input::MouseKey key);

  /**
   * @brief Retrieves the mouse axis delta value.
   *
   * @param [in] deviceId   Index of the device to check.
   * @param [in] key        Code of the axis to retrieve. It has to be either nau::input::MouseKey::AxisX or nau::input::MouseKey::AxisY.
   * @return                Difference between the current value of the axis and its value in the previous frame.
   */
  NAU_COREINPUT_EXPORT float getMouseAxisDelta(int deviceId, nau::input::MouseKey key);

  // Raw interface

  /**
   * @brief Retrieves the input manager.
   * 
   * @return A pointer to the input manager.
   */
  NAU_COREINPUT_EXPORT void* getGaInputManager();
}  // namespace nau::input
