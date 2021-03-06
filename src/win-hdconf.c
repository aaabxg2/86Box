/* Copyright holders: Sarah Walker, Tenshi
   see COPYING for more details
*/
#include <inttypes.h>
#define BITMAP WINDOWS_BITMAP
#include <windows.h>
#include <windowsx.h>
#undef BITMAP

#include "ibm.h"
#include "ide.h"
#include "resources.h"
#include "win.h"

static int hd_changed = 0;

static char hd_new_name[512];
static uint64_t hd_new_spt, hd_new_hpc, hd_new_cyl;
static int hd_new_hdi;
static int new_cdrom_channel;

static void update_hdd_cdrom(HWND hdlg)
{
        HWND h;
        
        h = GetDlgItem(hdlg, IDC_CHDD);
        SendMessage(h, BM_SETCHECK, (new_cdrom_channel == 0) ? 0 : 1, 0);
        h = GetDlgItem(hdlg, IDC_CCDROM);
        SendMessage(h, BM_SETCHECK, (new_cdrom_channel == 0) ? 1 : 0, 0);
        h = GetDlgItem(hdlg, IDC_DHDD);
        SendMessage(h, BM_SETCHECK, (new_cdrom_channel == 1) ? 0 : 1, 0);
        h = GetDlgItem(hdlg, IDC_DCDROM);
        SendMessage(h, BM_SETCHECK, (new_cdrom_channel == 1) ? 1 : 0, 0);
        h = GetDlgItem(hdlg, IDC_EHDD);
        SendMessage(h, BM_SETCHECK, (new_cdrom_channel == 2) ? 0 : 1, 0);
        h = GetDlgItem(hdlg, IDC_ECDROM);
        SendMessage(h, BM_SETCHECK, (new_cdrom_channel == 2) ? 1 : 0, 0);
        h = GetDlgItem(hdlg, IDC_FHDD);
        SendMessage(h, BM_SETCHECK, (new_cdrom_channel == 3) ? 0 : 1, 0);
        h = GetDlgItem(hdlg, IDC_FCDROM);
        SendMessage(h, BM_SETCHECK, (new_cdrom_channel == 3) ? 1 : 0, 0);
        h = GetDlgItem(hdlg, IDC_GCDROM);
        SendMessage(h, BM_SETCHECK, (new_cdrom_channel == 4) ? 1 : 0, 0);
        h = GetDlgItem(hdlg, IDC_HCDROM);
        SendMessage(h, BM_SETCHECK, (new_cdrom_channel == 5) ? 1 : 0, 0);
}

int hdnew_no_update = 0;

hard_disk_t hdnew_temp_hd;

int hdsize_no_update = 0;

hard_disk_t hdsize_temp_hd;

char s[260];

static int hdconf_initialize_hdt_combo(HWND hdlg, hard_disk_t *internal_hd)
{
        HWND h;
	int i = 0;
	uint64_t size = 0;
	uint64_t size_mb = 0;
	uint64_t size_shift = 11;
	int selection = 127;

	h = GetDlgItem(hdlg, IDC_COMBOHDT);
	for (i = 0; i < 127; i++)
	{	
		size = hdt[i][0] * hdt[i][1] * hdt[i][2];
		size_mb = size >> size_shift;
                sprintf(s, "%" PRIu64 " MB (CHS: %" PRIu64 ", %" PRIu64 ", %" PRIu64 ")", size_mb, hdt[i][0], hdt[i][1], hdt[i][2]);
		SendMessage(h, CB_ADDSTRING, 0, (LPARAM)s);
		if ((internal_hd->tracks == hdt[i][0]) && (internal_hd->hpc == hdt[i][1]) && (internal_hd->spt == hdt[i][2]))
		{
			selection = i;
		}
	}
	sprintf(s, "Custom...");
	SendMessage(h, CB_ADDSTRING, 0, (LPARAM)s);
	SendMessage(h, CB_SETCURSEL, selection, 0);
	return selection;
}

static void hdconf_update_text_boxes(HWND hdlg, BOOL enable)
{
	HWND h;

	h=GetDlgItem(hdlg, IDC_EDIT1);
	EnableWindow(h, enable);
	h=GetDlgItem(hdlg, IDC_EDIT2);
	EnableWindow(h, enable);
	h=GetDlgItem(hdlg, IDC_EDIT3);
	EnableWindow(h, enable);
	h=GetDlgItem(hdlg, IDC_EDIT4);
	EnableWindow(h, enable);
}

void hdconf_set_text_boxes(HWND hdlg, hard_disk_t *internal_hd)
{
	HWND h;

	uint64_t size_shift = 11;

	h = GetDlgItem(hdlg, IDC_EDIT1);
	sprintf(s, "%" PRIu64, internal_hd->spt);
	SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
	h = GetDlgItem(hdlg, IDC_EDIT2);
	sprintf(s, "%" PRIu64, internal_hd->hpc);
	SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
	h = GetDlgItem(hdlg, IDC_EDIT3);
	sprintf(s, "%" PRIu64, internal_hd->tracks);
	SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);

	h = GetDlgItem(hdlg, IDC_EDIT4);
	sprintf(s, "%" PRIu64, (internal_hd->spt * internal_hd->hpc * internal_hd->tracks) >> size_shift);
	SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
}

BOOL hdconf_initdialog_common(HWND hdlg, hard_disk_t *internal_hd, int *no_update, uint64_t spt, uint64_t hpc, uint64_t tracks)
{
	HWND h;
	int selection = 127;

	internal_hd->spt = spt;
	internal_hd->hpc = hpc;
	internal_hd->tracks = tracks;
	*no_update = 1;

	hdconf_set_text_boxes(hdlg, internal_hd);

	selection = hdconf_initialize_hdt_combo(hdlg, internal_hd);

	if (selection < 127)
	{
		hdconf_update_text_boxes(hdlg, FALSE);
	}
	else
	{
		hdconf_update_text_boxes(hdlg, TRUE);
	}

	*no_update = 0;

	return TRUE;
}

int hdconf_idok_common(HWND hdlg)
{
	HWND h;

	h = GetDlgItem(hdlg, IDC_EDIT1);
	SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
	sscanf(s, "%" PRIu64, &hd_new_spt);
	h = GetDlgItem(hdlg, IDC_EDIT2);
	SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
	sscanf(s, "%" PRIu64, &hd_new_hpc);
	h = GetDlgItem(hdlg, IDC_EDIT3);
	SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
	sscanf(s, "%" PRIu64, &hd_new_cyl);
                        
	if (hd_new_spt > 63)
	{
		MessageBox(ghwnd, "Drive has too many sectors (maximum is 63)", "86Box error", MB_OK);
		return 1;
	}
	if (hd_new_hpc > 16)
	{
		MessageBox(ghwnd, "Drive has too many heads (maximum is 16)", "86Box error", MB_OK);
		return 1;
	}
	if (hd_new_cyl > 16383)
	{
		MessageBox(ghwnd, "Drive has too many cylinders (maximum is 16383)", "86Box error", MB_OK);
		return 1;
	}

	return 0;
}

BOOL hdconf_process_edit_boxes(HWND hdlg, WORD control, uint64_t *var, hard_disk_t *internal_hd, int *no_update)
{
	HWND h;
	uint64_t size_shift = 11;

	if (*no_update)
	{
		return FALSE;
	}

	h = GetDlgItem(hdlg, control);
	SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
	sscanf(s, "%" PRIu64, var);

	*no_update = 1;
	if(!(*var))
	{
		*var = 1;
		sprintf(s, "%" PRIu64, *var);
		SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
	}

	if (control == IDC_EDIT4)
	{
		*var <<= 11;			/* Convert to sectors */
		*var /= internal_hd->hpc;
		*var /= internal_hd->spt;
		internal_hd->tracks = *var;

		h = GetDlgItem(hdlg, IDC_EDIT3);
		sprintf(s, "%" PRIu64, internal_hd->tracks);
		SendMessage(h, WM_SETTEXT, 1, (LPARAM)s);
	}
	else if ((control >= IDC_EDIT1) && (control <= IDC_EDIT3))
	{
		h = GetDlgItem(hdlg, IDC_EDIT4);
		sprintf(s, "%" PRIu64, (internal_hd->spt * internal_hd->hpc * internal_hd->tracks) >> size_shift);
		SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
	}

	*no_update = 0;
	return TRUE;
}

BOOL hdconf_process_hdt_combo(HWND hdlg, hard_disk_t *internal_hd, int *no_update, WPARAM wParam)
{
	HWND h;
	int selection = 127;

	if (*no_update)
	{
		return FALSE;
	}

	if (HIWORD(wParam) == CBN_SELCHANGE)
	{
		*no_update = 1;

		h = GetDlgItem(hdlg,IDC_COMBOHDT);
		selection = SendMessage(h,CB_GETCURSEL,0,0);

		if (selection < 127)
		{
			hdconf_update_text_boxes(hdlg, FALSE);

			internal_hd->tracks = hdt[selection][0];
			internal_hd->hpc = hdt[selection][1];
			internal_hd->spt = hdt[selection][2];

			hdconf_set_text_boxes(hdlg, internal_hd);
		}
		else
		{
			hdconf_update_text_boxes(hdlg, TRUE);
		}

		*no_update = 0;
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CALLBACK hdconf_common_dlgproc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam, int type, hard_disk_t *internal_hd, int *no_update, uint64_t spt, uint64_t hpc, uint64_t tracks)
{
        HWND h;
        uint64_t c;
        FILE *f;
        uint8_t buf[512];
	int is_hdi;
	uint64_t size;
	uint32_t zero = 0;
	uint32_t sector_size = 512;
	uint32_t base = 0x1000;
	uint64_t full_size = 0;
	uint64_t full_size_bytes = 0;
	uint64_t size_shift = 11;
	int selection = 127;
        switch (message)
        {
                case WM_INITDIALOG:
		if (!type)
		{
	                h = GetDlgItem(hdlg, IDC_EDITC);
        	        SendMessage(h, WM_SETTEXT, 0, (LPARAM)"");
		}
		return hdconf_initdialog_common(hdlg, internal_hd, no_update, spt, hpc, tracks);

                case WM_COMMAND:
                switch (LOWORD(wParam))
                {
                        case IDOK:
			if (!type)
			{
	                        h = GetDlgItem(hdlg, IDC_EDITC);
        	                SendMessage(h, WM_GETTEXT, 511, (LPARAM)hd_new_name);
                	        if (!hd_new_name[0])
                        	{
                                	MessageBox(ghwnd,"Please enter a valid filename","86Box error",MB_OK);
	                                return TRUE;
        	                }
			}

			if (hdconf_idok_common(hdlg))
			{
				return TRUE;
			}

			if (!type)
			{
                	        f = fopen64(hd_new_name, "wb");
        	                if (!f)
	                        {
                                	MessageBox(ghwnd, "Can't open file for write", "86Box error", MB_OK);
                        	        return TRUE;
                	        }
				full_size = (hd_new_cyl * hd_new_hpc * hd_new_spt);
				full_size_bytes = full_size * 512;
				if (image_is_hdi(hd_new_name))
				{
					if (full_size_bytes >= 0x100000000)
					{
						MessageBox(ghwnd, "Drive is HDI and 4 GB or bigger (size filed in HDI header is 32-bit)", "86Box error", MB_OK);
						fclose(f);
						return TRUE;
					}

					hd_new_hdi = 1;

					fwrite(&zero, 1, 4, f);			/* 00000000: Zero/unknown */
					fwrite(&zero, 1, 4, f);			/* 00000004: Zero/unknown */
					fwrite(&base, 1, 4, f);			/* 00000008: Offset at which data starts */
					fwrite(&full_size_bytes, 1, 4, f);	/* 0000000C: Full size of the data (32-bit) */
					fwrite(&sector_size, 1, 4, f);		/* 00000010: Sector size in bytes */
					fwrite(&hd_new_spt, 1, 4, f);		/* 00000014: Sectors per cylinder */
					fwrite(&hd_new_hpc, 1, 4, f);		/* 00000018: Heads per cylinder */
					fwrite(&hd_new_cyl, 1, 4, f);		/* 0000001C: Cylinders */

					for (c = 0; c < 0x3f8; c++)
					{
						fwrite(&zero, 1, 4, f);
					}
				}
	                        memset(buf, 0, 512);
                	        for (c = 0; c < full_size; c++)
        	                    fwrite(buf, 512, 1, f);
	                        fclose(f);
                        
	                        MessageBox(ghwnd, "Remember to partition and format the new drive", "86Box", MB_OK);
			}
                        
                        EndDialog(hdlg,1);
                        return TRUE;

                        case IDCANCEL:
                        EndDialog(hdlg, 0);
                        return TRUE;

                        case IDC_CFILE:
			if (!type)
			{
	                        if (!getsfile(hdlg, "Hard disc image (*.HDI;*.IMA;*.IMG)\0*.HDI;*.IMA;*.IMG\0All files (*.*)\0*.*\0", ""))
        	                {
                	                h = GetDlgItem(hdlg, IDC_EDITC);
                        	        SendMessage(h, WM_SETTEXT, 0, (LPARAM)openfilestring);
	                        }
	                        return TRUE;
			}
			else
			{
				break;
			}

                        case IDC_EDIT1:
			return hdconf_process_edit_boxes(hdlg, IDC_EDIT1, &(internal_hd->spt), internal_hd, no_update);

			case IDC_EDIT2:
			return hdconf_process_edit_boxes(hdlg, IDC_EDIT2, &(internal_hd->hpc), internal_hd, no_update);

			case IDC_EDIT3:
			return hdconf_process_edit_boxes(hdlg, IDC_EDIT3, &(internal_hd->tracks), internal_hd, no_update);

			case IDC_EDIT4:
			return hdconf_process_edit_boxes(hdlg, IDC_EDIT4, &size, internal_hd, no_update);

			case IDC_COMBOHDT:
			return hdconf_process_hdt_combo(hdlg, internal_hd, no_update, wParam);
                }
                break;

        }
        return FALSE;
}

static BOOL CALLBACK hdnew_dlgproc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	return hdconf_common_dlgproc(hdlg, message, wParam, lParam, 0, &hdnew_temp_hd, &hdnew_no_update, 63, 16, 511);
}

BOOL CALLBACK hdsize_dlgproc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	return hdconf_common_dlgproc(hdlg, message, wParam, lParam, 1, &hdsize_temp_hd, &hdsize_no_update, hd_new_spt, hd_new_hpc, hd_new_cyl);
}

static BOOL CALLBACK hdconf_dlgproc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
        HWND h;
        hard_disk_t hd[4];
        FILE *f;
        off64_t sz;
	int ret;
	uint32_t sector_size = 512;
	uint32_t base = 0x1000;
        switch (message)
        {
                case WM_INITDIALOG:
                pause = 1;
                hd[0] = hdc[0];
                hd[1] = hdc[1];
                hd[2] = hdc[2];
                hd[3] = hdc[3];
                hd_changed = 0;
                
                h = GetDlgItem(hdlg, IDC_EDIT_C_SPT);
                sprintf(s, "%" PRIu64, hdc[0].spt);
                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                h = GetDlgItem(hdlg, IDC_EDIT_C_HPC);
                sprintf(s, "%" PRIu64, hdc[0].hpc);
                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                h = GetDlgItem(hdlg, IDC_EDIT_C_CYL);
                sprintf(s, "%" PRIu64, hdc[0].tracks);
                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                h = GetDlgItem(hdlg, IDC_EDIT_C_FN);
                SendMessage(h, WM_SETTEXT, 0, (LPARAM)ide_fn[0]);

                h = GetDlgItem(hdlg, IDC_EDIT_D_SPT);
                sprintf(s, "%" PRIu64, hdc[1].spt);
                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                h = GetDlgItem(hdlg, IDC_EDIT_D_HPC);
                sprintf(s, "%" PRIu64, hdc[1].hpc);
                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                h = GetDlgItem(hdlg, IDC_EDIT_D_CYL);
                sprintf(s, "%" PRIu64, hdc[1].tracks);
                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                h=  GetDlgItem(hdlg, IDC_EDIT_D_FN);
                SendMessage(h, WM_SETTEXT, 0, (LPARAM)ide_fn[1]);
                
                h = GetDlgItem(hdlg, IDC_EDIT_E_SPT);
                sprintf(s, "%" PRIu64, hdc[2].spt);
                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                h = GetDlgItem(hdlg, IDC_EDIT_E_HPC);
                sprintf(s, "%" PRIu64, hdc[2].hpc);
                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                h = GetDlgItem(hdlg, IDC_EDIT_E_CYL);
                sprintf(s, "%" PRIu64, hdc[2].tracks);
                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                h=  GetDlgItem(hdlg, IDC_EDIT_E_FN);
                SendMessage(h, WM_SETTEXT, 0, (LPARAM)ide_fn[2]);
                
                h = GetDlgItem(hdlg, IDC_EDIT_F_SPT);
                sprintf(s, "%" PRIu64, hdc[3].spt);
                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                h = GetDlgItem(hdlg, IDC_EDIT_F_HPC);
                sprintf(s, "%" PRIu64, hdc[3].hpc);
                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                h = GetDlgItem(hdlg, IDC_EDIT_F_CYL);
                sprintf(s, "%" PRIu64, hdc[3].tracks);
                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                h=  GetDlgItem(hdlg, IDC_EDIT_F_FN);
                SendMessage(h, WM_SETTEXT, 0, (LPARAM)ide_fn[3]);
                
                h = GetDlgItem(hdlg, IDC_TEXT_C_SIZE);
                sprintf(s, "Size: %" PRIu64 " MB", (hd[0].tracks*hd[0].hpc*hd[0].spt) >> 11);
                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);

                h = GetDlgItem(hdlg, IDC_TEXT_D_SIZE);
                sprintf(s, "Size: %" PRIu64 " MB", (hd[1].tracks*hd[1].hpc*hd[1].spt) >> 11);
                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);

                h = GetDlgItem(hdlg, IDC_TEXT_E_SIZE);
                sprintf(s, "Size: %" PRIu64 " MB", (hd[2].tracks*hd[2].hpc*hd[2].spt) >> 11);
                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);

                h = GetDlgItem(hdlg, IDC_TEXT_F_SIZE);
                sprintf(s, "Size: %" PRIu64 " MB", (hd[3].tracks*hd[3].hpc*hd[3].spt) >> 11);
                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);

                new_cdrom_channel = atapi_cdrom_channel;

                update_hdd_cdrom(hdlg);
                return TRUE;
                
                case WM_COMMAND:
                switch (LOWORD(wParam))
                {
                        case IDOK:
                        if (hd_changed || atapi_cdrom_channel != new_cdrom_channel)
                        {                     
                                if (MessageBox(NULL, "This will reset 86Box!\nOkay to continue?", "86Box", MB_OKCANCEL) == IDOK)
                                {
                                        h = GetDlgItem(hdlg, IDC_EDIT_C_SPT);
                                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                                        sscanf(s, "%" PRIu64, &hd[0].spt);
                                        h = GetDlgItem(hdlg, IDC_EDIT_C_HPC);
                                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                                        sscanf(s, "%" PRIu64, &hd[0].hpc);
                                        h = GetDlgItem(hdlg, IDC_EDIT_C_CYL);
                                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                                        sscanf(s, "%" PRIu64, &hd[0].tracks);
                                        h = GetDlgItem(hdlg, IDC_EDIT_C_FN);
                                        SendMessage(h, WM_GETTEXT, 511, (LPARAM)ide_fn[0]);

                                        h = GetDlgItem(hdlg, IDC_EDIT_D_SPT);
                                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                                        sscanf(s, "%" PRIu64, &hd[1].spt);
                                        h = GetDlgItem(hdlg, IDC_EDIT_D_HPC);
                                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                                        sscanf(s, "%" PRIu64, &hd[1].hpc);
                                        h = GetDlgItem(hdlg, IDC_EDIT_D_CYL);
                                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                                        sscanf(s, "%" PRIu64, &hd[1].tracks);
                                        h = GetDlgItem(hdlg, IDC_EDIT_D_FN);
                                        SendMessage(h, WM_GETTEXT, 511, (LPARAM)ide_fn[1]);
                                        
                                        h = GetDlgItem(hdlg, IDC_EDIT_E_SPT);
                                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                                        sscanf(s, "%" PRIu64, &hd[2].spt);
                                        h = GetDlgItem(hdlg, IDC_EDIT_E_HPC);
                                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                                        sscanf(s, "%" PRIu64, &hd[2].hpc);
                                        h = GetDlgItem(hdlg, IDC_EDIT_E_CYL);
                                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                                        sscanf(s, "%" PRIu64, &hd[2].tracks);
                                        h = GetDlgItem(hdlg, IDC_EDIT_E_FN);
                                        SendMessage(h, WM_GETTEXT, 511, (LPARAM)ide_fn[2]);
                                        
                                        h = GetDlgItem(hdlg, IDC_EDIT_F_SPT);
                                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                                        sscanf(s, "%" PRIu64, &hd[3].spt);
                                        h = GetDlgItem(hdlg, IDC_EDIT_F_HPC);
                                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                                        sscanf(s, "%" PRIu64, &hd[3].hpc);
                                        h = GetDlgItem(hdlg, IDC_EDIT_F_CYL);
                                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                                        sscanf(s, "%" PRIu64, &hd[3].tracks);
                                        h = GetDlgItem(hdlg, IDC_EDIT_F_FN);
                                        SendMessage(h, WM_GETTEXT, 511, (LPARAM)ide_fn[3]);
                                        
                                        hdc[0] = hd[0];
                                        hdc[1] = hd[1];
                                        hdc[2] = hd[2];
                                        hdc[3] = hd[3];

                                        atapi_cdrom_channel = new_cdrom_channel;
                                        
                                        saveconfig();
                                                                                
                                        resetpchard();
                                }                                
                        }
                        case IDCANCEL:
                        EndDialog(hdlg, 0);
                        pause = 0;
                        return TRUE;

                        case IDC_EJECTC:
                        hd[0].spt = 0;
                        hd[0].hpc = 0;
                        hd[0].tracks = 0;
                        ide_fn[0][0] = 0;
                        SetDlgItemText(hdlg, IDC_EDIT_C_SPT, "0");
                        SetDlgItemText(hdlg, IDC_EDIT_C_HPC, "0");
                        SetDlgItemText(hdlg, IDC_EDIT_C_CYL, "0");
                        SetDlgItemText(hdlg, IDC_EDIT_C_FN, "");
                        hd_changed = 1;
                        return TRUE;
                        case IDC_EJECTD:
                        hd[1].spt = 0;
                        hd[1].hpc = 0;
                        hd[1].tracks = 0;
                        ide_fn[1][0] = 0;
                        SetDlgItemText(hdlg, IDC_EDIT_D_SPT, "0");
                        SetDlgItemText(hdlg, IDC_EDIT_D_HPC, "0");
                        SetDlgItemText(hdlg, IDC_EDIT_D_CYL, "0");
                        SetDlgItemText(hdlg, IDC_EDIT_D_FN, "");
                        hd_changed = 1;
                        return TRUE;
                        case IDC_EJECTE:
                        hd[2].spt = 0;
                        hd[2].hpc = 0;
                        hd[2].tracks = 0;
                        ide_fn[2][0] = 0;
                        SetDlgItemText(hdlg, IDC_EDIT_E_SPT, "0");
                        SetDlgItemText(hdlg, IDC_EDIT_E_HPC, "0");
                        SetDlgItemText(hdlg, IDC_EDIT_E_CYL, "0");
                        SetDlgItemText(hdlg, IDC_EDIT_E_FN, "");
                        hd_changed = 1;
                        return TRUE;
                        case IDC_EJECTF:
                        hd[3].spt = 0;
                        hd[3].hpc = 0;
                        hd[3].tracks = 0;
                        ide_fn[3][0] = 0;
                        SetDlgItemText(hdlg, IDC_EDIT_F_SPT, "0");
                        SetDlgItemText(hdlg, IDC_EDIT_F_HPC, "0");
                        SetDlgItemText(hdlg, IDC_EDIT_F_CYL, "0");
                        SetDlgItemText(hdlg, IDC_EDIT_F_FN, "");
                        hd_changed = 1;
                        return TRUE;
                        
                        case IDC_CNEW:
                        if (DialogBox(hinstance, TEXT("HdNewDlg"), hdlg, hdnew_dlgproc) == 1)
                        {
                                h = GetDlgItem(hdlg, IDC_EDIT_C_SPT);
                                sprintf(s, "%" PRIu64, hd_new_spt);
                                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                h = GetDlgItem(hdlg, IDC_EDIT_C_HPC);
                                sprintf(s, "%" PRIu64, hd_new_hpc);
                                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                h = GetDlgItem(hdlg, IDC_EDIT_C_CYL);
                                sprintf(s, "%" PRIu64, hd_new_cyl);
                                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                h = GetDlgItem(hdlg, IDC_EDIT_C_FN);
                                SendMessage(h, WM_SETTEXT, 0, (LPARAM)hd_new_name);

                                h = GetDlgItem(hdlg, IDC_TEXT_C_SIZE);
                                sprintf(s, "Size: %" PRIu64 " MB", (hd_new_cyl*hd_new_hpc*hd_new_spt) >> 11);
                                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);

                                hd_changed = 1;
                        }                              
                        return TRUE;
                        
                        case IDC_CFILE:
                        if (!getfile(hdlg, "Hard disc image (*.HDI;*.IMA;*.IMG;*.VHD)\0*.HDI;*.IMA;*.IMG;*.VHD\0All files (*.*)\0*.*\0", ""))
                        {
                                f = fopen64(openfilestring, "rb");
                                if (!f)
                                {
                                        MessageBox(ghwnd,"Can't open file for read","86Box error",MB_OK);
                                        return TRUE;
                                }

				if (image_is_hdi(openfilestring))
				{
					fseeko64(f, 0x10, SEEK_END);
					fread(&sector_size, 1, 4, f);
					if (sector_size != 512)
					{
						MessageBox(ghwnd,"HDI image with a sector size that is not 512","86Box error",MB_OK);
						fclose(f);
						return TRUE;
					}
					fread(&hd_new_spt, 1, 4, f);
					fread(&hd_new_hpc, 1, 4, f);
					fread(&hd_new_cyl, 1, 4, f);

					ret = 1;
				}
				else
				{
	                                fseeko64(f, -1, SEEK_END);
        	                        sz = ftello64(f) + 1;
                	                fclose(f);
	                                hd_new_spt = 63;
        	                        hd_new_hpc = 16;
                	                hd_new_cyl = ((sz / 512) / 16) / 63;

					ret = DialogBox(hinstance, TEXT("HdSizeDlg"), hdlg, hdsize_dlgproc);
                                }
                                if (ret == 1)
                                {
                                        h = GetDlgItem(hdlg, IDC_EDIT_C_SPT);
                                        sprintf(s, "%" PRIu64, hd_new_spt);
                                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                        h = GetDlgItem(hdlg, IDC_EDIT_C_HPC);
                                        sprintf(s, "%" PRIu64, hd_new_hpc);
                                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                        h = GetDlgItem(hdlg, IDC_EDIT_C_CYL);
                                        sprintf(s, "%" PRIu64, hd_new_cyl);
                                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                        h = GetDlgItem(hdlg, IDC_EDIT_C_FN);
                                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)openfilestring);

                                        h = GetDlgItem(hdlg, IDC_TEXT_C_SIZE);
                                        sprintf(s, "Size: %" PRIu64 " MB", (hd_new_cyl*hd_new_hpc*hd_new_spt) >> 11);
                                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
        
                                        hd_changed = 1;
                                }
                        }
                        return TRUE;
                                
                        case IDC_DNEW:
                        if (DialogBox(hinstance, TEXT("HdNewDlg"), hdlg, hdnew_dlgproc) == 1)
                        {
                                h = GetDlgItem(hdlg, IDC_EDIT_D_SPT);
                                sprintf(s, "%" PRIu64, hd_new_spt);
                                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                h = GetDlgItem(hdlg, IDC_EDIT_D_HPC);
                                sprintf(s, "%" PRIu64, hd_new_hpc);
                                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                h = GetDlgItem(hdlg, IDC_EDIT_D_CYL);
                                sprintf(s, "%" PRIu64, hd_new_cyl);
                                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                h = GetDlgItem(hdlg, IDC_EDIT_D_FN);
                                SendMessage(h, WM_SETTEXT, 0, (LPARAM)hd_new_name);

                                h=  GetDlgItem(hdlg, IDC_TEXT_D_SIZE);
                                sprintf(s, "Size: %" PRIu64 " MB", (hd_new_cyl*hd_new_hpc*hd_new_spt) >> 11);
                                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);

                                hd_changed = 1;
                        }                              
                        return TRUE;
                        
                        case IDC_DFILE:
                        if (!getfile(hdlg, "Hard disc image (*.HDI;*.IMA;*.IMG;*.VHD)\0*.HDI;*.IMA;*.IMG;*.VHD\0All files (*.*)\0*.*\0", ""))
                        {
                                f = fopen64(openfilestring, "rb");
                                if (!f)
                                {
                                        MessageBox(ghwnd,"Can't open file for read","86Box error",MB_OK);
                                        return TRUE;
                                }

				if (image_is_hdi(openfilestring))
				{
					fseeko64(f, 0x10, SEEK_END);
					fread(&sector_size, 1, 4, f);
					if (sector_size != 512)
					{
						MessageBox(ghwnd,"HDI image with a sector size that is not 512","86Box error",MB_OK);
						fclose(f);
						return TRUE;
					}
					fread(&hd_new_spt, 1, 4, f);
					fread(&hd_new_hpc, 1, 4, f);
					fread(&hd_new_cyl, 1, 4, f);

					ret = 1;
				}
				else
				{
	                                fseeko64(f, -1, SEEK_END);
        	                        sz = ftello64(f) + 1;
                	                fclose(f);
	                                hd_new_spt = 63;
        	                        hd_new_hpc = 16;
                	                hd_new_cyl = ((sz / 512) / 16) / 63;

					ret = DialogBox(hinstance, TEXT("HdSizeDlg"), hdlg, hdsize_dlgproc);
                                }
                                if (ret == 1)
                                {
                                        h = GetDlgItem(hdlg, IDC_EDIT_D_SPT);
                                        sprintf(s, "%" PRIu64, hd_new_spt);
                                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                        h = GetDlgItem(hdlg, IDC_EDIT_D_HPC);
                                        sprintf(s, "%" PRIu64, hd_new_hpc);
                                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                        h = GetDlgItem(hdlg, IDC_EDIT_D_CYL);
                                        sprintf(s, "%" PRIu64, hd_new_cyl);
                                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                        h = GetDlgItem(hdlg, IDC_EDIT_D_FN);
                                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)openfilestring);

                                        h = GetDlgItem(hdlg, IDC_TEXT_D_SIZE);
                                        sprintf(s, "Size: %" PRIu64 " MB", (hd_new_cyl*hd_new_hpc*hd_new_spt) >> 11);
                                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
        
                                        hd_changed = 1;
                                }
                        }
                        return TRUE;

                        case IDC_ENEW:
                        if (DialogBox(hinstance, TEXT("HdNewDlg"), hdlg, hdnew_dlgproc) == 1)
                        {
                                h = GetDlgItem(hdlg, IDC_EDIT_E_SPT);
                                sprintf(s, "%" PRIu64, hd_new_spt);
                                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                h = GetDlgItem(hdlg, IDC_EDIT_E_HPC);
                                sprintf(s, "%" PRIu64, hd_new_hpc);
                                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                h = GetDlgItem(hdlg, IDC_EDIT_E_CYL);
                                sprintf(s, "%" PRIu64, hd_new_cyl);
                                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                h = GetDlgItem(hdlg, IDC_EDIT_E_FN);
                                SendMessage(h, WM_SETTEXT, 0, (LPARAM)hd_new_name);

                                h=  GetDlgItem(hdlg, IDC_TEXT_E_SIZE);
                                sprintf(s, "Size: %" PRIu64 " MB", (hd_new_cyl*hd_new_hpc*hd_new_spt) >> 11);
                                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);

                                hd_changed = 1;
                        }                              
                        return TRUE;
                        
                        case IDC_EFILE:
                        if (!getfile(hdlg, "Hard disc image (*.HDI;*.IMA;*.IMG;*.VHD)\0*.HDI;*.IMA;*.IMG;*.VHD\0All files (*.*)\0*.*\0", ""))
                        {
                                f = fopen64(openfilestring, "rb");
                                if (!f)
                                {
                                        MessageBox(ghwnd,"Can't open file for read","86Box error",MB_OK);
                                        return TRUE;
                                }

				if (image_is_hdi(openfilestring))
				{
					fseeko64(f, 0x10, SEEK_END);
					fread(&sector_size, 1, 4, f);
					if (sector_size != 512)
					{
						MessageBox(ghwnd,"HDI image with a sector size that is not 512","86Box error",MB_OK);
						fclose(f);
						return TRUE;
					}
					fread(&hd_new_spt, 1, 4, f);
					fread(&hd_new_hpc, 1, 4, f);
					fread(&hd_new_cyl, 1, 4, f);

					ret = 1;
				}
				else
				{
	                                fseeko64(f, -1, SEEK_END);
        	                        sz = ftello64(f) + 1;
                	                fclose(f);
	                                hd_new_spt = 63;
        	                        hd_new_hpc = 16;
                	                hd_new_cyl = ((sz / 512) / 16) / 63;

					ret = DialogBox(hinstance, TEXT("HdSizeDlg"), hdlg, hdsize_dlgproc);
                                }
                                if (ret == 1)
                                {
                                        h = GetDlgItem(hdlg, IDC_EDIT_E_SPT);
                                        sprintf(s, "%" PRIu64, hd_new_spt);
                                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                        h = GetDlgItem(hdlg, IDC_EDIT_E_HPC);
                                        sprintf(s, "%" PRIu64, hd_new_hpc);
                                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                        h = GetDlgItem(hdlg, IDC_EDIT_E_CYL);
                                        sprintf(s, "%" PRIu64, hd_new_cyl);
                                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                        h = GetDlgItem(hdlg, IDC_EDIT_E_FN);
                                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)openfilestring);

                                        h = GetDlgItem(hdlg, IDC_TEXT_E_SIZE);
                                        sprintf(s, "Size: %" PRIu64 " MB", (hd_new_cyl*hd_new_hpc*hd_new_spt) >> 11);
                                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
        
                                        hd_changed = 1;
                                }
                        }
                        return TRUE;

                        case IDC_FNEW:
                        if (DialogBox(hinstance, TEXT("HdNewDlg"), hdlg, hdnew_dlgproc) == 1)
                        {
                                h = GetDlgItem(hdlg, IDC_EDIT_F_SPT);
                                sprintf(s, "%" PRIu64, hd_new_spt);
                                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                h = GetDlgItem(hdlg, IDC_EDIT_F_HPC);
                                sprintf(s, "%" PRIu64, hd_new_hpc);
                                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                h = GetDlgItem(hdlg, IDC_EDIT_F_CYL);
                                sprintf(s, "%" PRIu64, hd_new_cyl);
                                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                h = GetDlgItem(hdlg, IDC_EDIT_F_FN);
                                SendMessage(h, WM_SETTEXT, 0, (LPARAM)hd_new_name);

                                h=  GetDlgItem(hdlg, IDC_TEXT_F_SIZE);
                                sprintf(s, "Size: %" PRIu64 " MB", (hd_new_cyl*hd_new_hpc*hd_new_spt) >> 11);
                                SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);

                                hd_changed = 1;
                        }                              
                        return TRUE;
                        
                        case IDC_FFILE:
                        if (!getfile(hdlg, "Hard disc image (*.HDI;*.IMA;*.IMG;*.VHD)\0*.HDI;*.IMA;*.IMG;*.VHD\0All files (*.*)\0*.*\0", ""))
                        {
                                f = fopen64(openfilestring, "rb");
                                if (!f)
                                {
                                        MessageBox(ghwnd,"Can't open file for read","86Box error",MB_OK);
                                        return TRUE;
                                }

				if (image_is_hdi(openfilestring))
				{
					fseeko64(f, 0x10, SEEK_END);
					fread(&sector_size, 1, 4, f);
					if (sector_size != 512)
					{
						MessageBox(ghwnd,"HDI image with a sector size that is not 512","86Box error",MB_OK);
						fclose(f);
						return TRUE;
					}
					fread(&hd_new_spt, 1, 4, f);
					fread(&hd_new_hpc, 1, 4, f);
					fread(&hd_new_cyl, 1, 4, f);

					ret = 1;
				}
				else
				{
	                                fseeko64(f, -1, SEEK_END);
        	                        sz = ftello64(f) + 1;
                	                fclose(f);
	                                hd_new_spt = 63;
        	                        hd_new_hpc = 16;
                	                hd_new_cyl = ((sz / 512) / 16) / 63;

					ret = DialogBox(hinstance, TEXT("HdSizeDlg"), hdlg, hdsize_dlgproc);
                                }
                                if (ret == 1)
                                {
                                        h = GetDlgItem(hdlg, IDC_EDIT_F_SPT);
                                        sprintf(s, "%" PRIu64, hd_new_spt);
                                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                        h = GetDlgItem(hdlg, IDC_EDIT_F_HPC);
                                        sprintf(s, "%" PRIu64, hd_new_hpc);
                                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                        h = GetDlgItem(hdlg, IDC_EDIT_F_CYL);
                                        sprintf(s, "%" PRIu64, hd_new_cyl);
                                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                                        h = GetDlgItem(hdlg, IDC_EDIT_F_FN);
                                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)openfilestring);

                                        h = GetDlgItem(hdlg, IDC_TEXT_F_SIZE);
                                        sprintf(s, "Size: %" PRIu64 " MB", (hd_new_cyl*hd_new_hpc*hd_new_spt) >> 11);
                                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
        
                                        hd_changed = 1;
                                }
                        }
                        return TRUE;

                        case IDC_EDIT_C_SPT: case IDC_EDIT_C_HPC: case IDC_EDIT_C_CYL:
                        h = GetDlgItem(hdlg, IDC_EDIT_C_SPT);
                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                        sscanf(s, "%" PRIu64, &hd[0].spt);
                        h = GetDlgItem(hdlg, IDC_EDIT_C_HPC);
                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                        sscanf(s, "%" PRIu64, &hd[0].hpc);
                        h = GetDlgItem(hdlg, IDC_EDIT_C_CYL);
                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                        sscanf(s, "%" PRIu64, &hd[0].tracks);

                        h = GetDlgItem(hdlg, IDC_TEXT_C_SIZE);
                        sprintf(s, "Size: %" PRIu64 " MB", (hd[0].tracks*hd[0].hpc*hd[0].spt) >> 11);
                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                        return TRUE;

                        case IDC_EDIT_D_SPT: case IDC_EDIT_D_HPC: case IDC_EDIT_D_CYL:
                        h = GetDlgItem(hdlg, IDC_EDIT_D_SPT);
                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                        sscanf(s, "%" PRIu64, &hd[1].spt);
                        h = GetDlgItem(hdlg, IDC_EDIT_D_HPC);
                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                        sscanf(s, "%" PRIu64, &hd[1].hpc);
                        h = GetDlgItem(hdlg, IDC_EDIT_D_CYL);
                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                        sscanf(s, "%" PRIu64, &hd[1].tracks);

                        h = GetDlgItem(hdlg, IDC_TEXT_D_SIZE);
                        sprintf(s, "Size: %" PRIu64 " MB", (hd[1].tracks*hd[1].hpc*hd[1].spt) >> 11);
                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                        return TRUE;

                        case IDC_EDIT_E_SPT: case IDC_EDIT_E_HPC: case IDC_EDIT_E_CYL:
                        h = GetDlgItem(hdlg, IDC_EDIT_E_SPT);
                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                        sscanf(s, "%" PRIu64, &hd[2].spt);
                        h = GetDlgItem(hdlg, IDC_EDIT_E_HPC);
                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                        sscanf(s, "%" PRIu64, &hd[2].hpc);
                        h = GetDlgItem(hdlg, IDC_EDIT_E_CYL);
                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                        sscanf(s, "%" PRIu64, &hd[2].tracks);

                        h = GetDlgItem(hdlg, IDC_TEXT_E_SIZE);
                        sprintf(s, "Size: %" PRIu64 " MB", (hd[2].tracks*hd[2].hpc*hd[2].spt) >> 11);
                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                        return TRUE;

                        case IDC_EDIT_F_SPT: case IDC_EDIT_F_HPC: case IDC_EDIT_F_CYL:
                        h = GetDlgItem(hdlg, IDC_EDIT_F_SPT);
                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                        sscanf(s, "%" PRIu64, &hd[3].spt);
                        h = GetDlgItem(hdlg, IDC_EDIT_F_HPC);
                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                        sscanf(s, "%" PRIu64, &hd[3].hpc);
                        h = GetDlgItem(hdlg, IDC_EDIT_F_CYL);
                        SendMessage(h, WM_GETTEXT, 255, (LPARAM)s);
                        sscanf(s, "%" PRIu64, &hd[3].tracks);

                        h = GetDlgItem(hdlg, IDC_TEXT_F_SIZE);
                        sprintf(s, "Size: %" PRIu64 " MB", (hd[3].tracks*hd[3].hpc*hd[3].spt) >> 11);
                        SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
                        return TRUE;

                        case IDC_CHDD:
                        if (new_cdrom_channel == 0)
                                new_cdrom_channel = -1;
                        update_hdd_cdrom(hdlg);
                        return TRUE;
                        case IDC_DHDD:
                        if (new_cdrom_channel == 1)
                                new_cdrom_channel = -1;
                        update_hdd_cdrom(hdlg);
                        return TRUE;
                        case IDC_EHDD:
                        if (new_cdrom_channel == 2)
                                new_cdrom_channel = -1;
                        update_hdd_cdrom(hdlg);
                        return TRUE;
                        case IDC_FHDD:
                        if (new_cdrom_channel == 3)
                                new_cdrom_channel = -1;
                        update_hdd_cdrom(hdlg);
                        return TRUE;
                        
                        case IDC_CCDROM:
                        new_cdrom_channel = 0;
                        update_hdd_cdrom(hdlg);
                        return TRUE;
                        case IDC_DCDROM:
                        new_cdrom_channel = 1;
                        update_hdd_cdrom(hdlg);
                        return TRUE;
                        case IDC_ECDROM:
                        new_cdrom_channel = 2;
                        update_hdd_cdrom(hdlg);
                        return TRUE;
                        case IDC_FCDROM:
                        new_cdrom_channel = 3;
                        update_hdd_cdrom(hdlg);
                        return TRUE;                        
                        case IDC_GCDROM:
                        new_cdrom_channel = 4;
                        update_hdd_cdrom(hdlg);
                        return TRUE;                        
                        case IDC_HCDROM:
                        new_cdrom_channel = 5;
                        update_hdd_cdrom(hdlg);
                        return TRUE;                        
                }
                break;

        }
        return FALSE;
}

void hdconf_open(HWND hwnd)
{
        DialogBox(hinstance, TEXT("HdConfDlg"), hwnd, hdconf_dlgproc);
}        
