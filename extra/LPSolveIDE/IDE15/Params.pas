unit Params;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls;

type
  TParamForm = class(TForm)
    FileName: TEdit;
    Button1: TButton;
    ProfilName: TComboBox;
    Label1: TLabel;
    Label2: TLabel;
    OpenDialog: TOpenDialog;
    Button2: TButton;
    Button3: TButton;
    Button5: TButton;
    procedure FormCreate(Sender: TObject);
    procedure Button1Click(Sender: TObject);
    procedure ProfilNameDropDown(Sender: TObject);
//    procedure Button5Click(Sender: TObject);
    procedure Button3Click(Sender: TObject);
    procedure Button2Click(Sender: TObject);
//    procedure Button4Click(Sender: TObject);

  private
    { Déclarations privées }

  public
    { Déclarations publiques }
  end;

var
  ParamForm: TParamForm;

implementation
uses XPMenu, inifiles,  main;

{$R *.dfm}

procedure TParamForm.FormCreate(Sender: TObject);
var  FCurrentFile: string;
begin        // persist filename to mainform since this form is dynamic
  TXPMenu.Create(self).Active := true;
  FileName.text := Mainform.GetlpParamsFile;
  ProfilName.SetTextBuf('IDElast');
  If (FileName.text = '') then
    begin
      FCurrentFile := Mainform.GetCurrentFile;
      if FCurrentFile <> '' then
         FCurrentFile := ChangeFileExt(FCurrentFile, '.lpi');
      FileName.text := FCurrentFile; //'Bill.lpi';
      Mainform.SetlpParamsFile(  FCurrentFile);
    end;
end;

procedure TParamForm.Button1Click(Sender: TObject);
begin     // FileName find button
  if OpenDialog.Execute then
    FileName.Text := OpenDialog.FileName;
end;

procedure TParamForm.ProfilNameDropDown(Sender: TObject);
var ini: TIniFile;
begin
  If (FileName.Text = '') then
    begin
        ProfilName.Items.Clear;
    end;
  if FileExists(FileName.Text) then
  begin
    ini := TIniFile.Create(FileName.Text);
    try
      ini.ReadSections(ProfilName.Items);
    finally
      ini.Free;
    end;
  end;
end;

procedure TParamForm.Button2Click(Sender: TObject);
begin   // Read Button
  MainForm.acLoadParamsExecute(self);
end;
procedure TParamForm.Button3Click(Sender: TObject);
begin    // Write Button   -  hand off to LP
  MainForm.acSaveParamsExecute(self);
end;

{procedure TParamForm.Button4Click(Sender: TObject);
begin   // Reset Button
  MainForm.acResetOptionsExecute(self);
end;
)
{procedure TParamForm.Button5Click(Sender: TObject);
begin  // Cancel button   - do nothing

end;
}
end.
