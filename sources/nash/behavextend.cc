//
// $Source$
// $Date$
// $Revision$
//
// DESCRIPTION:
// Algorithms for extending behavior profiles to Nash equilibria
//
// This file is part of Gambit
// Copyright (c) 2002, The Gambit Project
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

#include "behavextend.h"
#include "poly/ineqsolv.h"

//
// Design choice: the auxiliary functions here are made static
// rather than members to help hide the gPoly-related details of
// the implementation.  Some of these functions might be more
// generally useful, in which case they should be made visible
// somehow.  Also, a namespace would be preferable to using
// static, but static is used for portability.  -- TLT, 5/2001.
//

//=========================================================================
//                      class algExtendsToNash
//=========================================================================

static void DeviationInfosets(gList<Infoset *> &answer,
			      const EFSupport & big_supp,
			      const EFPlayer *pl,
			      const Node* node,
			      const Action *act)
{
  Node *child  = node->GetChild(act);
  if ( child->IsNonterminal() ) {
    Infoset *iset = child->GetInfoset();
    if ( iset->GetPlayer() == pl ) {
      int insert = 0;
      bool done = false;
      while (!done) {
	insert ++;
	if (insert > answer.Length() ||
	    iset->Precedes(answer[insert]->GetMember(1)))
	  done = true;
      }
      answer.Insert(iset,insert);
    }
    gList<Action *> action_list = iset->ListOfActions();
    for (int j = 1; j <= action_list.Length(); j++) {
      DeviationInfosets(answer,big_supp,pl,child,action_list[j]);
    }
  }
}

static gList<Infoset *> DeviationInfosets(const EFSupport &big_supp,
					  const EFPlayer *pl,
					  const Infoset *iset,
					  const Action *act)
{
  gList<Infoset *> answer;
  
  gList<const Node *> node_list = iset->ListOfMembers();
  for (int i = 1; i <= node_list.Length(); i++) {
    DeviationInfosets(answer,big_supp,pl,node_list[i],act);
  }

  return answer;
}

static gPolyList<gDouble> 
ActionProbsSumToOneIneqs(const BehavSolution &p_solution,
			 const gSpace &BehavStratSpace, 
			 const term_order &Lex,
			 const EFSupport &big_supp,
			 const gList<gList<int> > &var_index) 
{
  gPolyList<gDouble> answer(&BehavStratSpace, &Lex);

  for (int pl = 1; pl <= p_solution.GetGame().NumPlayers(); pl++) 
    for (int i = 1; i <= p_solution.GetGame().Players()[pl]->NumInfosets(); i++) {
      Infoset *current_infoset = p_solution.GetGame().Players()[pl]->Infosets()[i];
      if ( !big_supp.HasActiveActionAt(current_infoset) ) {
	int index_base = var_index[pl][i];
	gPoly<gDouble> factor(&BehavStratSpace, (gDouble)1.0, &Lex);
	for (int k = 1; k < current_infoset->NumActions(); k++)
	  factor -= gPoly<gDouble>(&BehavStratSpace, index_base + k, 1, &Lex);
	answer += factor;
      }
    }
  return answer;
}

static gList<EFSupport> 
DeviationSupports(const EFSupport & big_supp,
		  const gList<Infoset *> & isetlist,
		  const EFPlayer */*pl*/,
		  const Infoset */*iset*/,
		  const Action */*act*/)
{
  gList<EFSupport> answer;

  gArray<int> active_act_no(isetlist.Length());

  for (int k = 1; k <= active_act_no.Length(); k++)
    active_act_no[k] = 0;
 
  EFSupport new_supp(big_supp);

  for (int i = 1; i <= isetlist.Length(); i++) {
    for (int j = 1; j < isetlist[i]->NumActions(); j++)
      new_supp.RemoveAction(isetlist[i]->GetAction(j));
    new_supp.AddAction(isetlist[i]->GetAction(1));

    active_act_no[i] = 1;
    for (int k = 1; k < i; k++)
      if (isetlist[k]->Precedes(isetlist[i]->GetMember(1)))
	if (isetlist[k]->GetAction(1)->Precedes(isetlist[i]->GetMember(1))) {
	  new_supp.RemoveAction(isetlist[i]->GetAction(1));
	  active_act_no[i] = 0;
	}
  }
  answer += new_supp;

  int iset_cursor = isetlist.Length();
  while (iset_cursor > 0) {
    if ( active_act_no[iset_cursor] == 0 || 
	 active_act_no[iset_cursor] == isetlist[iset_cursor]->NumActions() )
      iset_cursor--;
    else {
      new_supp.RemoveAction(isetlist[iset_cursor]->
			    GetAction(active_act_no[iset_cursor]));
      active_act_no[iset_cursor]++;
      new_supp.AddAction(isetlist[iset_cursor]->
			 GetAction(active_act_no[iset_cursor]));
      for (int k = iset_cursor + 1; k <= isetlist.Length(); k++) {
	if (active_act_no[k] > 0)
	  new_supp.RemoveAction(isetlist[k]->GetAction(1));
	int h = 1;
	bool active = true;
	while (active && h < k) {
	  if (isetlist[h]->Precedes(isetlist[k]->GetMember(1)))
	    if (active_act_no[h] == 0 || 
		!isetlist[h]->GetAction(active_act_no[h])->
	              Precedes(isetlist[k]->GetMember(1))) {
	      active = false;
	      if (active_act_no[k] > 0) {
		new_supp.RemoveAction(isetlist[k]->
				      GetAction(active_act_no[k]));
		active_act_no[k] = 0;
	      }
	    }
	  h++;
	}
	if (active){
	  new_supp.AddAction(isetlist[k]->GetAction(1));
	  active_act_no[k] = 1;
	}
      }
      answer += new_supp;
    }
  }
  return answer;
}

static bool 
NashNodeProbabilityPoly(const BehavSolution &p_solution,
			gPoly<gDouble> & node_prob,
			const gSpace &BehavStratSpace, 
			const term_order &Lex,
			const EFSupport &dsupp,
			const gList<gList<int> > &var_index,
			const Node *tempnode,
			const EFPlayer */*pl*/,
			const Infoset *iset,
			const Action *act)
{
  while (tempnode != p_solution.GetGame().RootNode()) {

    const Action *last_action = tempnode->GetAction();
    Infoset *last_infoset = last_action->BelongsTo();
    
    if (last_infoset->IsChanceInfoset()) 
      node_prob *= (gDouble) p_solution.GetGame().GetChanceProb(last_action);
    else 
      if (dsupp.HasActiveActionAt(last_infoset)) {
	if (last_infoset == iset) {
	  if (act != last_action) {
	    return false;
	  }
	}
	else
	  if (dsupp.ActionIsActive((Action *)last_action)) {
	    if ( last_action->BelongsTo()->GetPlayer() !=
		         act->BelongsTo()->GetPlayer()     ||
		 !act->Precedes(tempnode) )
	    node_prob *= (gDouble) p_solution.ActionProb(last_action);
	  }
	  else {
	    return false;
	  }
      }
      else {
	int initial_var_no = 
 var_index[last_infoset->GetPlayer()->GetNumber()][last_infoset->GetNumber()];
	if (last_action->GetNumber() < last_infoset->NumActions()){
	  int varno = initial_var_no + last_action->GetNumber();
	  node_prob *= gPoly<gDouble>(&BehavStratSpace, varno, 1, &Lex);
	}
	else {
	  gPoly<gDouble> factor(&BehavStratSpace, (gDouble)1.0, &Lex);
	  int k;
	  for (k = 1; k < last_infoset->NumActions(); k++)
	    factor -= gPoly<gDouble>(&BehavStratSpace,
				     initial_var_no + k, 1, &Lex);
	  node_prob *= factor;
	}
      } 
    tempnode = tempnode->GetParent();
  }
  return true;
}

static gPolyList<gDouble> 
NashExpectedPayoffDiffPolys(const BehavSolution &p_solution,
			    const gSpace &BehavStratSpace, 
			    const term_order &Lex,
			    const EFSupport &little_supp,
			    const EFSupport &big_supp,
			    const gList<gList<int> > &var_index) 
{
  gPolyList<gDouble> answer(&BehavStratSpace, &Lex);

  gList<Node *> terminal_nodes = p_solution.GetGame().TerminalNodes();

  const gArray<EFPlayer *> players = p_solution.GetGame().Players();
  for (int pl = 1; pl <= players.Length(); pl++) {
    const gArray<Infoset *> isets_for_pl = players[pl]->Infosets();
    for (int i = 1; i <= isets_for_pl.Length(); i++) {
      if (little_supp.MayReach(isets_for_pl[i])) {
	const gArray<Action *> acts_for_iset = isets_for_pl[i]->Actions();
	for (int j = 1; j <= acts_for_iset.Length(); j++)
	  if ( !little_supp.ActionIsActive(acts_for_iset[j]) ) {
	    gList<Infoset *> isetlist = DeviationInfosets(big_supp, 
							  players[pl],
							  isets_for_pl[i],
							  acts_for_iset[j]);
	    gList<EFSupport> dsupps = DeviationSupports(big_supp, 
							isetlist, 
							players[pl],
							isets_for_pl[i],
							acts_for_iset[j]);
	    for (int k = 1; k <= dsupps.Length(); k++) {

	    // This will be the utility difference between the
	    // payoff resulting from the profile and deviation to 
	    // the strategy for pl specified by dsupp[k]

	      gPoly<gDouble> next_poly(&BehavStratSpace, &Lex);

	      for (int n = 1; n <= terminal_nodes.Length(); n++) {
		gPoly<gDouble> node_prob(&BehavStratSpace, (gDouble)1.0, &Lex);
		if (NashNodeProbabilityPoly(p_solution, node_prob,
					    BehavStratSpace,
					    Lex,
					    dsupps[k],
					    var_index,
					    terminal_nodes[n],
					    players[pl],
					    isets_for_pl[i],
					    acts_for_iset[j])) {
		  node_prob *= 
		    (gDouble) p_solution.GetGame().Payoff(terminal_nodes[n],
						       p_solution.GetGame().Players()[pl]);
		  next_poly += node_prob;
		}
	      }
	      answer += -next_poly + (gDouble) p_solution.Payoff(pl);
	    }
	  }
      }
    }
  }
  return answer;
}

static gPolyList<gDouble> 
ExtendsToNashIneqs(const BehavSolution &p_solution,
		   const gSpace &BehavStratSpace, 
		   const term_order &Lex,
		   const EFSupport &little_supp,
		   const EFSupport &big_supp,
		   const gList<gList<int> > &var_index)
{
  gPolyList<gDouble> answer(&BehavStratSpace, &Lex);
  answer += ActionProbsSumToOneIneqs(p_solution, BehavStratSpace, 
				     Lex, 
				     big_supp, 
				     var_index);

  answer += NashExpectedPayoffDiffPolys(p_solution, BehavStratSpace, 
				    Lex, 
				    little_supp,
				    big_supp,
				    var_index);
  return answer;
}

bool algExtendsToNash::ExtendsToNash(const BehavSolution &p_solution,
				     const EFSupport &little_supp,
				     const EFSupport &big_supp,
				     gStatus &m_status)
{
  // This asks whether there is a Nash extension of the BehavSolution to 
  // all information sets at which the behavioral probabilities are not
  // specified.  The assumption is that the support has active actions
  // at infosets at which the behavioral probabilities are defined, and
  // no others.  Also, the BehavSol is assumed to be already a Nash
  // equilibrium for the truncated game obtained by eliminating stuff
  // outside little_supp.
  
  // First we compute the number of variables, and indexing information
  int num_vars(0);
  gList<gList<int> > var_index;
  int pl;
  for (pl = 1; pl <= p_solution.GetGame().NumPlayers(); pl++) {

    gList<int> list_for_pl;

    for (int i = 1; i <= p_solution.GetGame().Players()[pl]->NumInfosets(); i++) {
      list_for_pl += num_vars;
      if ( !big_supp.HasActiveActionAt(p_solution.GetGame().Players()[pl]->Infosets()[i]) ) {
	num_vars += p_solution.GetGame().Players()[pl]->Infosets()[i]->NumActions() - 1;
      }
    }
    var_index += list_for_pl;
  }

  // We establish the space
  gSpace BehavStratSpace(num_vars);
  ORD_PTR ptr = &lex;
  term_order Lex(&BehavStratSpace, ptr);

  num_vars = BehavStratSpace.Dmnsn();

  gPolyList<gDouble> inequalities = ExtendsToNashIneqs(p_solution,
						       BehavStratSpace,
						       Lex,
						       little_supp,
						       big_supp,
						       var_index);
  // set up the rectangle of search
  gVector<gDouble> bottoms(num_vars), tops(num_vars);
  bottoms = (gDouble)0;
  tops = (gDouble)1;
  gRectangle<gDouble> Cube(bottoms, tops); 

  // Set up the test and do it
  IneqSolv<gDouble> extension_tester(inequalities,m_status);
  gVector<gDouble> sample(num_vars);
  bool answer = extension_tester.ASolutionExists(Cube,sample); 
  
  //  assert (answer == m_profile->ExtendsToNash(little_supp, big_supp, m_status));

  return answer;
}


//=========================================================================
//                   class algExtendsToAgentNash
//=========================================================================

static bool ANFNodeProbabilityPoly(const BehavSolution &p_solution,
				   gPoly<gDouble> & node_prob,
				   const gSpace &BehavStratSpace, 
				   const term_order &Lex,
				   const EFSupport &big_supp,
				   const gList<gList<int> > &var_index,
				   const Node *tempnode,
				   const int &pl,
				   const int &i,
				   const int &j)
{
  while (tempnode != p_solution.GetGame().RootNode()) {
    const Action *last_action = tempnode->GetAction();
    Infoset *last_infoset = last_action->BelongsTo();
    
    if (last_infoset->IsChanceInfoset()) 
      node_prob *= (gDouble)p_solution.GetGame().GetChanceProb(last_action);
    else 
      if (big_supp.HasActiveActionAt(last_infoset)) {
	if (last_infoset == p_solution.GetGame().Players()[pl]->Infosets()[i]) {
	  if (j != last_action->GetNumber()) 
	    return false;
	}
	else
	  if (big_supp.ActionIsActive((Action *)last_action))
	    node_prob *= (gDouble) p_solution.ActionProb(last_action);
	  else 
	    return false;
      }
      else {
	int initial_var_no = 
 var_index[last_infoset->GetPlayer()->GetNumber()][last_infoset->GetNumber()];
	if (last_action->GetNumber() < last_infoset->NumActions()){
	  int varno = initial_var_no + last_action->GetNumber();
	  node_prob *= gPoly<gDouble>(&BehavStratSpace, varno, 1, &Lex);
	}
	else {
	  gPoly<gDouble> factor(&BehavStratSpace, (gDouble)1.0, &Lex);
	  int k;
	  for (k = 1; k < last_infoset->NumActions(); k++)
	    factor -= gPoly<gDouble>(&BehavStratSpace,
				     initial_var_no + k, 1, &Lex);
	  node_prob *= factor;
	}
      } 
    tempnode = tempnode->GetParent();
  }
  return true;
}

static gPolyList<gDouble> 
ANFExpectedPayoffDiffPolys(const BehavSolution &p_solution,
			   const gSpace &BehavStratSpace, 
			   const term_order &Lex,
			   const EFSupport &little_supp,
			   const EFSupport &big_supp,
			   const gList<gList<int> > &var_index)
{
  gPolyList<gDouble> answer(&BehavStratSpace, &Lex);

  gList<Node *> terminal_nodes = p_solution.GetGame().TerminalNodes();

  for (int pl = 1; pl <= p_solution.GetGame().NumPlayers(); pl++)
    for (int i = 1; i <= p_solution.GetGame().Players()[pl]->NumInfosets(); i++) {
      Infoset *infoset = p_solution.GetGame().Players()[pl]->Infosets()[i];
      if (little_supp.MayReach(infoset)) 
	for (int j = 1; j <= infoset->NumActions(); j++)
	  if (!little_supp.ActionIsActive(pl,i,j)) {
	
	    // This will be the utility difference between the
	    // payoff resulting from the profile and deviation to 
	    // action j
	    gPoly<gDouble> next_poly(&BehavStratSpace, &Lex);

	    for (int n = 1; n <= terminal_nodes.Length(); n++) {
	      gPoly<gDouble> node_prob(&BehavStratSpace, (gDouble)1.0, &Lex);
	      if (ANFNodeProbabilityPoly(p_solution, node_prob,
					 BehavStratSpace,
					 Lex,
					 big_supp,
					 var_index,
					 terminal_nodes[n],
					 pl,i,j)) {
		node_prob *= 
		  (gDouble)p_solution.GetGame().Payoff(terminal_nodes[n],
					 p_solution.GetGame().Players()[pl]);
		next_poly += node_prob;
	      }
	    }
	    answer += -next_poly + (gDouble) p_solution.Payoff(pl);
	  }
    }
  return answer;
}

static gPolyList<gDouble> 
ExtendsToANFNashIneqs(const BehavSolution &p_solution,
		      const gSpace &BehavStratSpace, 
		      const term_order &Lex,
		      const EFSupport &little_supp,
		      const EFSupport &big_supp,
		      const gList<gList<int> > &var_index)
{
  gPolyList<gDouble> answer(&BehavStratSpace, &Lex);
  answer += ActionProbsSumToOneIneqs(p_solution, BehavStratSpace, 
				     Lex, 
				     big_supp, 
				     var_index);
  answer += ANFExpectedPayoffDiffPolys(p_solution, BehavStratSpace, 
				       Lex, 
				       little_supp,
				       big_supp,
				       var_index);
  return answer;
}

bool algExtendsToAgentNash::ExtendsToAgentNash(const BehavSolution &p_solution,
					       const EFSupport &little_supp,
					       const EFSupport &big_supp,
					       gStatus &p_status)
{
  // This asks whether there is an ANF Nash extension of the BehavSolution to 
  // all information sets at which the behavioral probabilities are not
  // specified.  The assumption is that the support has active actions
  // at infosets at which the behavioral probabilities are defined, and
  // no others.
  
  // First we compute the number of variables, and indexing information
  int num_vars(0);
  gList<gList<int> > var_index;
  int pl;
  for (pl = 1; pl <= p_solution.GetGame().NumPlayers(); pl++) {

    gList<int> list_for_pl;

    for (int i = 1; i <= p_solution.GetGame().Players()[pl]->NumInfosets(); i++) {
      list_for_pl += num_vars;
      if ( !big_supp.HasActiveActionAt(p_solution.GetGame().Players()[pl]->Infosets()[i]) ) {
	num_vars += p_solution.GetGame().Players()[pl]->Infosets()[i]->NumActions() - 1;
      }
    }
    var_index += list_for_pl;
  }

  // We establish the space
  gSpace BehavStratSpace(num_vars);
  ORD_PTR ptr = &lex;
  term_order Lex(&BehavStratSpace, ptr);

  num_vars = BehavStratSpace.Dmnsn();
  gPolyList<gDouble> inequalities = ExtendsToANFNashIneqs(p_solution,
							  BehavStratSpace,
							  Lex,
							  little_supp,
							  big_supp,
							  var_index);

  // set up the rectangle of search
  gVector<gDouble> bottoms(num_vars), tops(num_vars);
  bottoms = (gDouble)0;
  tops = (gDouble)1;
  gRectangle<gDouble> Cube(bottoms, tops); 

  // Set up the test and do it
  IneqSolv<gDouble> extension_tester(inequalities, p_status);
  gVector<gDouble> sample(num_vars);

  // Temporarily, we check the old set up vs. the new
  bool ANFanswer = extension_tester.ASolutionExists(Cube,sample); 
  //  assert (ANFanswer == m_profile->ExtendsToANFNash(little_supp,
  //						   big_supp,
  //						   m_status));

  /* 
  bool NASHanswer = m_profile->ExtendsToNash(Support(),Support(),m_status);

  //DEBUG
  if (ANFanswer && !NASHanswer)
    gout << 
      "The following should be extendable to an ANF Nash, but not to a Nash:\n"
	 << *m_profile << "\n\n";
  if (NASHanswer && !ANFanswer)
    gout << 
      "ERROR: said to be extendable to a Nash, but not to an ANF Nash:\n"
	 << *m_profile << "\n\n";
	  */
  return ANFanswer;
}