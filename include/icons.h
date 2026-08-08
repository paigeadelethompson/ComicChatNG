#pragma once

#include <QIcon>
#include <QString>

// Icons cropped from the original Comic Chat bitmap strips (res/).
// Each cell is 16x16; the resource names mirror the old IDB_ strips:
//   "toolbar_N"  -> Main toolbar buttons
//   "texttool_N" -> Text toolbar buttons
//   "usertool_N" -> Member toolbar buttons
//   "member_N"   -> Member-list small pictures
//   "saybar_N"   -> Say-bar balloon pictures
namespace icons
{
    // Returns an icon from a ":" relative resource path ("/icons/...").
    QIcon fromResource(const QString &cell);

    QIcon main(int index);      // toolbar_N
    QIcon text(int index);      // texttool_N
    QIcon user(int index);      // usertool_N
    QIcon member(int index);    // member_N
    QIcon saybar(int index);    // saybar_N
    QIcon window();             // chat.png
    QIcon room();               // room.png
} // namespace icons