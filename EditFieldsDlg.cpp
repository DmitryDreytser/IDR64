//----------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "EditFieldsDlg.h"
#include "Misc.h"

//----------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

//extern  bool        ProjectModified;

#define     FD_OP_EDIT      0
#define     FD_OP_ADD       1
#define     FD_OP_DELETE    2

TFEditFieldsDlg_11011981 *FEditFieldsDlg_11011981;
//----------------------------------------------------------------------------
__fastcall TFEditFieldsDlg_11011981::TFEditFieldsDlg_11011981(TComponent *Owner)
	: TForm(Owner)
{
    Op = FD_OP_EDIT;
    SelIndex = -1;
    FieldOffset = -1;
    VmtAdr = 0;
    fieldsList = new TList;
}
//----------------------------------------------------------------------------
__fastcall TFEditFieldsDlg_11011981::~TFEditFieldsDlg_11011981()
{
	if (fieldsList) delete fieldsList;
}
//----------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::ShowFields()
{
    lbFields->Enabled = true;
    lbFields->Clear();
    fieldsList->Clear();

    int fieldsNum = LoadFieldTable(VmtAdr, fieldsList);
    if (fieldsNum)
    {
        SelIndex = -1;
    	for (int n = 0; n < fieldsNum; n++)
        {
            PFIELDINFO fInfo = (PFIELDINFO)fieldsList->Items[n];
            String line = Val2Str5(fInfo->Offset) + " ";
            if (fInfo->Name != "")
            	line += fInfo->Name;
            else
            	line += "?";
            line += ":";
            if (fInfo->Type != "")
            	line += fInfo->Type;
            else
            	line += "?";
            lbFields->Items->Add(line);
            if (fInfo->Offset == FieldOffset)
                SelIndex = n;
        }
    }
}
//----------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::FormShow(TObject *Sender)
{
	Caption = GetClsName(VmtAdr) + " fields";

    edtPanel->Visible = false;
    lbFields->Height = lbFXrefs->Height;

    lbFXrefs->Clear();
    ShowFields();
    lbFields->ItemIndex = SelIndex;

    bEdit->Enabled = (lbFields->Count && lbFields->ItemIndex != -1);
    bAdd->Enabled = true;
    bRemove->Enabled = (lbFields->Count && lbFields->ItemIndex != -1);
    bClose->Enabled = true;

    TypModified = false;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::lbFXrefsDblClick(TObject *Sender)
{
    char            type[2];
    DWORD           adr;

    String item = lbFXrefs->Items->Strings[lbFXrefs->ItemIndex];
    sscanf(item.c_str() + 1, "%lX%2c", &adr, type);

    for (int m = Adr2Pos(adr); m >= 0; m--)
    {
        if (idr.IsFlagSet(cfProcStart, m))
        {
            //FMain_11011981->ShowCode(Pos2Adr(m), adr, -1, -1);
            ::SendMessage(Application->MainForm->Handle, WM_SHOWCODE, 0,
                    (long)new ShowCodeData(Pos2Adr(m), adr, -1, -1));

            break;
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::lbFieldsClick(TObject *Sender)
{
    lbFXrefs->Clear();

    if (lbFields->ItemIndex == -1) return;

    String line;
    PFIELDINFO fInfo = (PFIELDINFO)fieldsList->Items[lbFields->ItemIndex];
    if (fInfo->xrefs)
    {
        for (int n = 0; n < fInfo->xrefs->Count; n++)
        {
            PXrefRec recX = (PXrefRec)fInfo->xrefs->Items[n];
            line = Val2Str8(recX->adr + recX->offset);
            if (recX->type == 'c') line += " <-";
            lbFXrefs->Items->Add(line);
        }
    }
    bEdit->Enabled = (lbFields->Count && lbFields->ItemIndex != -1);
    bRemove->Enabled = (lbFields->Count && lbFields->ItemIndex != -1);
}
//---------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::bEditClick(TObject *Sender)
{
    Op = FD_OP_EDIT;
	lbFields->Height = lbFXrefs->Height - edtPanel->Height;
    edtPanel->Visible = true;

    PFIELDINFO fInfo = (PFIELDINFO)fieldsList->Items[lbFields->ItemIndex];
    edtOffset->Text = Val2Str0(fInfo->Offset);
    edtOffset->Enabled = false;
    edtName->Text = fInfo->Name;
    edtName->Enabled = true;
    edtType->Text = fInfo->Type;
    edtType->Enabled = true;
    edtName->SetFocus();

    lbFields->Enabled = false;
    bApply->Enabled = false;
    bClose->Enabled = true;
    bEdit->Enabled = false;
    bAdd->Enabled = false;
    bRemove->Enabled = false;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::edtNameChange(TObject *Sender)
{
	bApply->Enabled = true;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::edtTypeChange(TObject *Sender)
{
	bApply->Enabled = true;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::bApplyClick(TObject *Sender)
{
    bool        vmt, ok = false;
    DWORD       adr;
    int         offset;
    PInfoRec    recN;
    PFIELDINFO  fInfo;
    String      text;

    switch (Op)
    {
    case FD_OP_EDIT:
        fInfo = (PFIELDINFO)fieldsList->Items[lbFields->ItemIndex];
        fInfo->Name = edtName->Text;
        fInfo->Type = edtType->Text;
        break;
    case FD_OP_ADD:
    case FD_OP_DELETE:
        text = edtOffset->Text;
        sscanf(text.c_str(), "%lX", &offset);
        recN = GetInfoRec(VmtAdr);
        if (Op == FD_OP_ADD)
        {
            fInfo = GetField(recN->GetName(), offset, &vmt, &adr);
            if (!fInfo || Application->MessageBox("Field already exists", "Replace?", MB_YESNO) == IDYES)
                recN->vmtInfo->AddField(0, 0, FIELD_PUBLIC, offset, -1, edtName->Text, edtType->Text);
        }
        else if (Op == FD_OP_DELETE)
        {
            if (Application->MessageBox("Delete field?", "Confirmation", MB_YESNO) == IDYES)
                recN->vmtInfo->RemoveField(offset);
        }
        break;
    }
    int itemidx = lbFields->ItemIndex;
    int topidx = lbFields->TopIndex;
    ShowFields();
    lbFields->ItemIndex = itemidx;
    lbFields->TopIndex = topidx;

    ::SendMessage(Application->MainForm->Handle, WM_SHOWCODE, 3, 0);

    ::SendMessage(Application->MainForm->Handle, WM_SHOWCLASSVIEWER, 0, VmtAdr);

    edtPanel->Visible = false;
    lbFields->Height = lbFXrefs->Height;

    lbFields->Enabled = true;
    bEdit->Enabled = (lbFields->Count && lbFields->ItemIndex != -1);
    bAdd->Enabled = true;
    bRemove->Enabled = (lbFields->Count && lbFields->ItemIndex != -1);

    TypModified = true;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::bCloseClick(
      TObject *Sender)
{
    edtPanel->Visible = false;
    lbFields->Height = lbFXrefs->Height;
    
    lbFields->Enabled = true;
    bEdit->Enabled = (lbFields->Count && lbFields->ItemIndex != -1);
    bAdd->Enabled = true;
    bRemove->Enabled = (lbFields->Count && lbFields->ItemIndex != -1);
}
//---------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::bAddClick(TObject *Sender)
{
    Op = FD_OP_ADD;
	lbFields->Height = lbFXrefs->Height - edtPanel->Height;
    edtPanel->Visible = true;

    edtOffset->Text = "";
    edtOffset->Enabled = true;
    edtName->Text = "";
    edtName->Enabled = true;
    edtType->Text = "";
    edtType->Enabled = true;
    edtOffset->SetFocus();

    lbFields->Enabled = false;
    bApply->Enabled = false;
    bClose->Enabled = true;
    bEdit->Enabled = false;
    bAdd->Enabled = false;
    bRemove->Enabled = false;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::bRemoveClick(TObject *Sender)
{
    Op = FD_OP_DELETE;
	lbFields->Height = lbFXrefs->Height - edtPanel->Height;
    edtPanel->Visible = true;

    PFIELDINFO fInfo = (PFIELDINFO)fieldsList->Items[lbFields->ItemIndex];
    edtOffset->Text = Val2Str0(fInfo->Offset);
    edtOffset->Enabled = false;
    edtName->Text = fInfo->Name;
    edtName->Enabled = false;
    edtType->Text = fInfo->Type;
    edtType->Enabled = false;

    lbFields->Enabled = false;
    bApply->Enabled = true;
    bClose->Enabled = true;
    bEdit->Enabled = false;
    bAdd->Enabled = false;
    bRemove->Enabled = false;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::FormCreate(TObject *Sender)
{
    ScaleForm(this);
}
//---------------------------------------------------------------------------

