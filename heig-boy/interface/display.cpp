#include "display.h"

// D�clarations de la table des �v�nements
// Sorte de relation qui lit des identifiants d'�v�nements aux fonctions
BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_BUTTON(ID_Bt_Click, MyFrame::OnClickButton1)
END_EVENT_TABLE();


// Code de l'initialisation de l'application
bool MyApp::OnInit()
{
  // On cr�e une instance de la classe MyFrame
  // On d�finit le texte qui apparait en haurt puis son emplacement et sa taille.
  MyFrame *frame = new MyFrame( "Hello World", wxPoint(50,50), wxSize(450,340) );
  // On la rend visible
  frame->Show(TRUE);
  SetTopWindow(frame);
  return TRUE;
} 


// Construction de la fen�tre. Elle ne contient qu'un bouton.
MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
: wxFrame((wxFrame *)NULL, -1, title, pos, size)
{
  // Cr�ation d'un bouton. Ce bouton est associ� � l'identifiant 
  // �v�nement ID_Bt_Click, en consultant, la table des �v�nements
  // on en d�duit que c'est la fonction OnClickButton qui sera 
  // appel�e lors d'un click sur ce bouton
  MonBouton1 = new wxButton(this,ID_Bt_Click," Cliquez moi dessus !");
}


// Fonction qui est ex�cut�e lors du click sur le bouton.
void MyFrame::OnClickButton1(wxCommandEvent& WXUNUSED(event))
{
    // Affiche une boite de dialogue avec la chaine "C'est un Hello world...
    wxMessageBox("C'est un Hello world  wxWindows par Nico ",
        "Hello World", wxOK | wxICON_INFORMATION, this);
}