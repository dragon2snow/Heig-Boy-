#include "wx/wx.h"

#include "CaptureDialog.h"
#include "CapturePanel.h"

BEGIN_EVENT_TABLE(CaptureDialog, wxDialog)
	EVT_CLOSE(CaptureDialog::onWindowClose)
END_EVENT_TABLE()

void CaptureDialog::onWindowClose(wxCloseEvent &event)
{
	//Pas de capture
	EndModal(wxID_CANCEL);
}



CaptureDialog::CaptureDialog(wxWindow* parent)
{
	Create(parent, wxID_ANY, wxT("Waiting for input..."),
           wxDefaultPosition, wxDefaultSize,
           wxCAPTION | wxSYSTEM_MENU | wxCLOSE_BOX | wxWANTS_CHARS);
	
	new CapturePanel(this);

	//Redimmensionnement et centrage
	SetClientSize(200,100);
	Centre();
}

long CaptureDialog::getKeyPressed()
{
	return this->keyCode;
}

IMPLEMENT_CLASS(CaptureDialog, wxDialog)