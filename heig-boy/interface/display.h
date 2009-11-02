// Indispensable pour faire des wxwidgets :
#include "wx/wx.h" 

// On doit créer un identifiant pour chaque évenement
// Ceci permettra, par exemple, d'associer un même
// évemenement à deux boutons
#define ID_Bt_Click 1

//Déclaration d'une classe MyApp (Mon application) dérivée de wxApp
class MyApp: public wxApp
{
    virtual bool OnInit();
};


//Déclaration d'une classe MyFrame (Ma fenetre principale) dérivée de wxFrame
class MyFrame: public wxFrame
{
public:
    //Constructeur de la fenetre :
    MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size); 

    //Fonction qui sera appelé lorsque l'utilisateur cliquera sur le MonBouton1
    void OnClickButton1(wxCommandEvent& event);

    // Boutton 1
    wxButton *MonBouton1;

    // C'est la table qui est écrite dans le fichier cpp
    DECLARE_EVENT_TABLE()
};

// Sorte de main ...
IMPLEMENT_APP(MyApp)