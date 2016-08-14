#ifndef BOBCONNECTERROR_DIALOG_H
#define BOBCONNECTERROR_DIALOG_H

#include <QDialog>

class BobErrorDialog: public QDialog
{
    Q_OBJECT

  private:
    QDialog * ErrorConnectBobDialog;

  public:
    BobErrorDialog (QWidget * parent = 0);
    ~BobErrorDialog () {}
    QWidget * createAboutTab ();
    QWidget * createAuthorsTab ();
    QWidget * createLicenseTab ();

  private slots:
	void showDialogModal()
	{
		// on affiche la boite de dialogue de facon modale
		dlg->setModal(true);
		dlg->show();
	}

	private:
	Dialog *dlg;
};	

};

#endif