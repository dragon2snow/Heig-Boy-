#include <wx/wx.h>

#include "CapturePanel.h"
#include "CaptureDialog.h"

BEGIN_EVENT_TABLE(CapturePanel, wxPanel)
    EVT_KEY_DOWN(CapturePanel::onKeyDown)
    EVT_PAINT(CapturePanel::onPaint)
END_EVENT_TABLE()

CapturePanel::CapturePanel(CaptureDialog *parent) 
{
	Create(parent, wxID_ANY,wxDefaultPosition,wxDefaultSize,wxWANTS_CHARS);
	this->SetBackgroundColour(_("white"));
}


void CapturePanel::onKeyDown(wxKeyEvent &event) {

	CaptureDialog &dlg = static_cast<CaptureDialog &>(*GetParent());
        
    // save the keycode
    dlg.keyCode = event.m_keyCode;
    
    // return wxID_OK
    dlg.EndModal(wxID_OK);
}

void CapturePanel::onPaint(wxPaintEvent &) {
    CaptureDialog &dlg = static_cast<CaptureDialog &>(*GetParent());
    
    // create a device context
    wxPaintDC dc(this);
    
    // draw the label
    dc.DrawText(_("Press any key..."), 25, 40);
}

IMPLEMENT_CLASS(CapturePanel, wxPanel)

