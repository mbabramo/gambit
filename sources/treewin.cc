//*************************************************************************
//* Treewin.cc: This file contains the type-specific (templated) portion of
//* the extensive form rendering code.
// @(#)treewin.cc	1.18 7/4/95
//
#pragma hdrstop
#include "wx.h"
#include "wx_form.h"
#include "wxmisc.h"
#include "extform.h"
#include "infoset.h"
#include "node.h"
#include "gmisc.h"
#include "treewin.h"
#include "extshow.h"


template <class T>
char *OutcomeToString(const gVector<T> &v,const TreeDrawSettings &draw_settings)
{
static gString gvts;
gvts="(";
ToStringPrecision(2);
for (int i=v.First();i<=v.Last();i++)
{
	if (i!=1) gvts+=",";
	if (draw_settings.ColorCodedOutcomes())
		gvts+=("\\C{"+ToString(draw_settings.GetPlayerColor(i))+"}");
	gvts+=ToString(v[i]);
}
if (draw_settings.ColorCodedOutcomes()) gvts+=("\\C{"+ToString(WX_COLOR_LIST_LENGTH-1)+"}");
gvts+=")";

return (char *)gvts;
}



//=====================================================================
//                      TREEWINDOW MEMBER FUNCTIONS
//=====================================================================

//---------------------------------------------------------------------
//                TREEWINDOW: CONSTRUCTOR AND DESTRUCTOR
//---------------------------------------------------------------------
//************************* private constructor & assignment *******
template <class T>
TreeWindow<T>::TreeWindow(const TreeWindow<T> &t):BaseTreeWindow(t),ef(t.ef)
{assert(0);}
template <class T>
void TreeWindow<T>::operator=(const TreeWindow<T> &t)
{assert(0);}


//************************* normal constructor ********************
template <class T>
TreeWindow<T>::TreeWindow(ExtForm<T> &ef_,ExtensiveShow<T> *frame_,
												int _subgame,int x,int y,int w,int h,int style)
												:	ef(ef_), frame(frame_), BaseTreeWindow(ef_,frame_,x,y,w,h,style)
{ }

template <class T>
TreeWindow<T>::~TreeWindow()
{
#ifdef SUBGAMES
if (subgame!=1) ;		// if I am a subgame, notify parent of my death
//	 ((ExtensiveShow<T> *)frame->parent)->tw->CloseSubgame(subgame);
else
#endif
delete &ef;
}

template <class T>
double TreeWindow<T>::ProbAsDouble(const Node *n,int action)
{
return (double)frame->GetActionProb(n,action);
}

template <class T>
gString TreeWindow<T>::ProbAsString(const Node *n,int action)
{
T prob=frame->GetActionProb(n,action);
if (prob<(T)0) return ""; else return ToString(prob);
}

template <class T>
gString TreeWindow<T>::OutcomeAsString(const Node *n)
{
if (n->GetOutcome())
{
	Outcome *t=n->GetOutcome();
	OutcomeVector<T> *tv=(OutcomeVector<T> *)t;
	gVector<T> *ttv=(gVector<T> *)tv;
	return OutcomeToString(*ttv,draw_settings);
}
return "";
}

//***********************************************************************
//                      TREE-OUTCOME MENU HANDLER
//***********************************************************************
// If save_now is true, we just update the outcome data in the extensive
// form, but do not delete the dialog
#include "outcomed.h"
template<class T> void TreeWindow<T>::tree_outcomes(const gString out_name,int save_num)
{
int i,j,out=1;
static OutcomeDialog *outcome_dialog=0;
static gBlock<gString> old_names;
int num_players=ef.NumPlayers();
if (!outcome_dialog)	// creating a new one
{
	// Check if there are any players to build outcomes for
	if (num_players<1)
		{wxMessageBox("No players exist, can not build an outcome","Error",wxOK | wxCENTRE,frame);return;}
	int rows=(ef.NumOutcomes()) ? ef.NumOutcomes() : 1;
	int cols=num_players;
	// figure out the # of this outcome
	if (out_name!=gString())
		for (i=1;i<=ef.NumOutcomes();i++) if ((ef.OutcomeList()[i])->GetName()==out_name) out=i;
	// create the dialog

	outcome_dialog=new OutcomeDialog(rows,cols,(BaseTreeWindow *)this,(wxFrame *)frame,out);
	for (i=1;i<=num_players;i++)
		outcome_dialog->SetLabelCol(i,(ef.PlayerList()[i])->GetName());
	old_names=gBlock<gString>(ef.NumOutcomes());
	for (i=1;i<=ef.NumOutcomes();i++)
	{
		OutcomeVector<T> *tmp=(OutcomeVector<T> *)(ef.OutcomeList()[i]);
		for (int j=1;j<=num_players;j++)
			outcome_dialog->SetCell(i,j,ToString((*tmp)[j]));
		outcome_dialog->SetCell(i,cols+1,tmp->GetName());
		old_names[i]=tmp->GetName();
	}
	outcome_dialog->Redraw();
	outcome_dialog->Show(TRUE);
	return;
}
else	// either going to a new one by clicking on an outcome, closing, or saving
{
	if (out_name!="")	// setting a new row
	{
		// figure out the # of this outcome
		for (i=1;i<=ef.NumOutcomes();i++) if ((ef.OutcomeList()[i])->GetName()==out_name) out=i;
		outcome_dialog->SetCurRow(out);return;
	}
	else	// this is an OK, or it is a save action
	{
		if (save_num)	// save action
		{
			T dummy;
			// if a new row was created, append an entry to old_names.
			if (save_num>ef.NumOutcomes()) old_names+=gString();
			OutcomeVector<T> *tmp=(save_num>ef.NumOutcomes()) ? ef.NewOutcome() : ef.GetOutcome(old_names[save_num]);
			if (tmp)
			{
				for (j=1;j<=num_players;j++)
						(*tmp)[j]=FromString(outcome_dialog->GetCell(save_num,j),dummy);
				if (!outcome_dialog->EnteredCell(save_num,j) || outcome_dialog->GetCell(save_num,j)=="")
				{
					outcome_dialog->SetCell(save_num,num_players+1,"Outcome "+ToString(save_num));
					outcome_dialog->OnPaint();
				}
				tmp->SetName(outcome_dialog->GetCell(save_num,num_players+1));
				old_names[save_num]=tmp->GetName();
				OnPaint();
			}
		}
		else	// OK action
		{
			delete outcome_dialog;
			outcome_dialog=0;
		}
	}
}
}

//***********************************************************************
//                      NODE-PROBS MENU HANDLER
//***********************************************************************
#define ENTRIES_PER_ROW	8

template<class T> void TreeWindow<T>::action_probs(void)
{
Node *n=cursor;
int 	i,num_children=n->NumChildren();

if (!n->GetPlayer()->IsChance())	// if this is not a chance player
{
	wxMessageBox("Probabilities only valid for CHANCE player","Error",wxOK | wxCENTRE,frame);
	return;
}

MyDialogBox *node_probs_dialog=new MyDialogBox(frame,"Node Probabilities");
ToStringPrecision(4);
char **prob_vector=new char *[num_children+1];
for (i=1;i<=num_children;i++)
{
	T temp_p=((ChanceInfoset<T> *)(n->GetInfoset()))->GetActionProb(i);
	prob_vector[i]=new char[20];
	strcpy(prob_vector[i],ToString(temp_p));
	node_probs_dialog->Form()->Add(wxMakeFormString("",&(prob_vector[i]),wxFORM_TEXT,NULL,NULL,wxVERTICAL,80));
	if (i%ENTRIES_PER_ROW==0) node_probs_dialog->Add(wxMakeFormNewLine());
}
node_probs_dialog->Go();
if (node_probs_dialog->Completed()==wxOK)
{
	T dummy;
	for (i=1;i<=num_children;i++)
		((ChanceInfoset<T> *)n->GetInfoset())->SetActionProb(i,FromString(prob_vector[i],dummy));

}
for (i=1;i<=num_children;i++) delete [] prob_vector[i];delete [] prob_vector;
delete node_probs_dialog;
}

//***********************************************************************
//                      FILE-SAVE MENU HANDLER
//***********************************************************************
template <class T> void TreeWindow<T>::file_save(void)
{
char *s=wxFileSelector("Save data file", 0, 0, 0, "*.efg");
if (s)
{
	gFileOutput out((const char *)s);
	ef.WriteEfgFile(out);
}
}



#ifdef __GNUG__
	#define TEMPLATE template
#elif defined __BORLANDC__
	#pragma option -Jgd
	#define TEMPLATE
#endif   // __GNUG__, __BORLANDC__
TEMPLATE class TreeWindow<double> ;
TEMPLATE char *OutcomeToString(const gVector<double> &v,const TreeDrawSettings &draw_settings);

#ifdef GRATIONAL
	TEMPLATE class TreeWindow<gRational> ;
	TEMPLATE char *OutcomeToString(const gVector<gRational> &v,const TreeDrawSettings &draw_settings);
#endif

