  Dimensions

  | Element                 | Width      | Height                      | Position               |
  |-------------------------|------------|-----------------------------|------------------------|
  | Display                 | 800px      | 480px                       | -                      |
  | Top Bar (Status)        | 800px      | 44px                        | y=0, separator at y=44 |
  | Printer Card            | ~492px*    | 130px                       | x=16, y=52             |
  | NFC/Scale Card          | ~492px*    | 125px                       | x=16, y=190 (52+138)   |
  | Action Buttons          | 130px each | 130px (top), 125px (bottom) | 2x2 grid, right side   |
  | AMS Cards (L/R)         | 380px each | 118px                       | y=323                  |
  | Bottom Notification Bar | 768px      | 30px                        | y=449 (323+118+8)      |

  *Printer/NFC card width is calculated: 800 - 16 - 8 - 130 - 8 - 130 - 16 = 492px

  Button Grid (Right Side)

  - Gap between buttons: 8px
  - Top row: "AMS Setup", "Encode Tag" (height: 130px)
  - Bottom row: "Catalog", "Settings" (height: 125px)

  ---
  Color Codes

  | Name             | Hex     | Usage                    |
  |------------------|---------|--------------------------|
  | COLOR_BG         | #1A1A1A | Main background          |
  | COLOR_CARD       | #2D2D2D | Card backgrounds         |
  | COLOR_BORDER     | #3D3D3D | Card borders, separators |
  | COLOR_ACCENT     | #00FF00 | Primary accent (green)   |
  | COLOR_WHITE      | #FFFFFF | Primary text             |
  | COLOR_GRAY       | #808080 | Secondary text           |
  | COLOR_TEXT_MUTED | #707070 | Muted/disabled text      |
  | COLOR_STATUS_BAR | #1A1A1A | Status bar background    |

  Additional Colors Used

  - Warning/Orange: #FFA500
  - Error/Red: #FF4444
  - Inner shadow bg: #0A0A0A
  - Highlight border: #505050

  ---
  Spacing

  - XS: 4px
  - SM: 8px
  - MD: 16px
  - LG: 24px (used between card and weight display)
  - XL: 32px

