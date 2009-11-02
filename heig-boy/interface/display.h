// Indispensable pour faire des wxwidgets :
#include "wx/wx.h" 

// On doit cr�er un identifiant pour chaque �venement
// Ceci permettra, par exemple, d'associer un m�me
// �vemenement � deux boutons
#define ID_Bt_Click 1

//D�claration d'une classe MyApp (Mon application) d�riv�e de wxApp
class MyApp: public wxApp
{
    virtual bool OnInit();
};


//D�claration d'une classe MyFrame (Ma fenetre principale) d�riv�e de wxFrame
class MyFrame: public wxFrame
{
public:
    //Constructeur de la fenetre :
    MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size); 

    //Fonction qui sera appel� lorsque l'utilisateur cliquera sur le MonBouton1
    void OnClickButton1(wxCommandEvent& event);

    // Boutton 1
    wxButton *MonBouton1;

    // C'est la table qui est �crite dans le fichier cpp
    DECLARE_EVENT_TABLE()
};

// Sorte de main ...
IMPLEMENT_APP(MyApp)