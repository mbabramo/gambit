//
// $Source$
// $Date$
// $Revision$
//
// DESCRIPTION:
// Dialog for setting labels for extensive form display
//
// This file is part of Gambit
// Copyright (c) 2005, The Gambit Project
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//

#ifndef DIALOG_TREE_LABEL_H
#define DIALOG_TREE_LABEL_H

#include "tree-layout.h"

class gbtTreeLabelDialog : public wxDialog {
private:
  wxRadioBox *m_nodeAbove, *m_nodeBelow, *m_outcome;
  wxRadioBox *m_branchAbove, *m_branchBelow;

public:
  // Lifecycle
  gbtTreeLabelDialog(wxWindow *, const gbtTreeLayoutOptions &);

  // Data access (only valid when ShowModal() returns with wxID_OK)
  int GetNodeAboveLabel(void) const { return m_nodeAbove->GetSelection(); }
  int GetNodeBelowLabel(void) const { return m_nodeBelow->GetSelection(); }
  int GetOutcomeLabel(void) const { return m_outcome->GetSelection(); }

  int GetBranchAboveLabel(void) const { return m_branchAbove->GetSelection(); }
  int GetBranchBelowLabel(void) const { return m_branchBelow->GetSelection(); }
};

#endif // DIALOG_TREE_LABEL_H