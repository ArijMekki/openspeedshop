////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2005 Silicon Graphics, Inc. All Rights Reserved.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your option)
// any later version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
// details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this library; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
////////////////////////////////////////////////////////////////////////////////


#ifndef STATSPANEL_H
#define STATSPANEL_H
#include "StatsPanelBase.hxx"           // Do not remove
#include "Panel.hxx"           // Do not remove

class PanelContainer;   // Do not remove
class QVBoxLayout;

#include <qlistview.h>
#include <qinputdialog.h>

#include "CollectorInfo.hxx"   // For dummied up data...

#include "SPListViewItem.hxx"

#include <qvaluelist.h>
typedef QValueList<QString> ColumnList;

#include <qsettings.h>

#define PANEL_CLASS_NAME StatsPanel   // Change the value of the define
//! StatsPanel Class
class StatsPanel  : public StatsPanelBase
{
    //! Q_OBJECT is needed as there are slots defined for the class
    Q_OBJECT
  public:
    //! StatsPanel(PanelContainer *pc, const char *name)
    StatsPanel(PanelContainer *pc, const char *n, void *argument);

    //! ~StatsPanel() - The default destructor.
    ~StatsPanel();  // Active destructor

    //! Calls the user panel function listener() request.
    int listener(void *msg);

  protected:
    //! Sets the language specific strings.
    virtual void languageChange();

  private:
};
#endif // STATSPANEL_H
