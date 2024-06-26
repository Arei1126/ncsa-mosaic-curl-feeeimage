#include "../config.h"
#include "mosaic.h"
#include <FreeImage.h>
#include <X11/Xos.h>
#include "readIMG.h"
#include "../libwww2/HTAlert.h"

#ifndef DISABLE_TRACE
extern int srcTrace;
#endif

void FreeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char *message) {

#ifndef DISABLE_TRACE
	if(srcTrace){
	fprintf(stderr,"\n*** ");
		if(fif != FIF_UNKNOWN) {
			fprintf(stderr,"%s Format\n", FreeImage_GetFormatFromFIF(fif));
		}
		fprintf(stderr,message);
		fprintf(stderr," ***\n");
	}
#endif
}

unsigned char *ReadIMG_name(char *name, int *w, int*h, XColor *c, int *bg){
#ifndef DISABLE_TRACE
	if(srcTrace){
		fprintf(stderr,"[ReadIMG_name] Entering\n");
	}
#endif

	FreeImage_Initialise(FALSE);
	FreeImage_SetOutputMessage(FreeImageErrorHandler);

	unsigned char *pixmap;	
	XColor *colors = c;


	FIBITMAP *bitmap;

	int transparent_index = -1;


	FREE_IMAGE_FORMAT filetype = FreeImage_GetFileType(name,0);
	if(filetype == -1){
#ifndef DISABLE_TRACE
		if(srcTrace){
			fprintf(stderr,"[ReadIMG_name] Could not identify file type\n");
		}
#endif	
		FreeImage_DeInitialise();
		return (unsigned char *)NULL;

	}
#ifndef DISABLE_TRACE
	if(srcTrace){
		fprintf(stderr,"[ReadIMG_name] filetype: %d\n",filetype);
	}
#endif

	bitmap = FreeImage_Load(filetype,name,0);

	if(bitmap == NULL){
#ifndef DISABLE_TRACE
		if(srcTrace){
			fprintf(stderr,"[ReadIMG_name] First attempt FreeImage_LoadFromHandle err format = %d\n",filetype);

		}
#endif
		FreeImage_DeInitialise();
		return (unsigned char *)NULL;
	}



	// 画像の幅と高さを取得
	*w = FreeImage_GetWidth(bitmap);
	*h = FreeImage_GetHeight(bitmap);
#ifndef DISABLE_TRACE
	if(srcTrace){
		fprintf(stderr,"size: %d x %d\n",*w,*h);
	}
#endif




	if((*w)%4 != 0){		// Image with width not x4, FreeImage has storane behavior	
	if(FreeImage_IsTransparent(bitmap)){
		transparent_index = 0;
#ifndef DISABLE_TRACE
	if(srcTrace){
		fprintf(stderr,"Original Image is transparent\n");
	}
#endif	
	}
		int nw = 4 - ((*w)%4) + (*w);
		FIBITMAP *nb;
		nb = FreeImage_Rescale(bitmap, nw,*h,FILTER_BOX);
		//FILTER_BOX
		//FILTER_BILINEAR
		//FILTER_BSPLINE 4th order (cubic) B-Spline
		//FILTER_BICUBIC Mitchell and Netravali's two-param cubic filter
		//FILTER_CATMULLROM Catmull-Rom spline, Overhauser spline
		//FILTER_LANCZOS3a
		if(nb == NULL){
#ifndef DISABLE_TRACE
			if(srcTrace){
				fprintf(stderr,"[ReadIMG_name]Rescale failed\n");
			}
#endif
			FreeImage_Unload(bitmap);
			FreeImage_DeInitialise();
			return NULL;
		}
		FreeImage_Unload(bitmap);
		bitmap = nb;
		*w = nw;
#ifndef DISABLE_TRACE
		if(srcTrace){
			fprintf(stderr,"Rescaled to: %d x %d\n",nw,*h);
		}
#endif
		/*
		int ret = FreeImage_Save(filetype,nb,name,0);
		if(ret == FALSE){
#ifndef DISABLE_TRACE
		if(srcTrace){
			fprintf(stderr,"Cannnot save scaled img\n");
		}
#endif
			FreeImage_Unload(bitmap);
			FreeImage_Unload(nb);
			FreeImage_DeInitialise();
			return NULL;
		}			
		FreeImage_Unload(bitmap);
		FreeImage_Unload(nb);
		bitmap = NULL;
		nb = NULL;
		FreeImage_DeInitialise();
		return ReadIMG_name(name, w, h, c, bg);  // restart
							 
		 */
	}

	int bpp = FreeImage_GetBPP(bitmap);
#ifndef DISABLE_TRACE
	if(srcTrace){
		fprintf(stderr,"bpp before: %d\n",bpp);
	}
#endif


	if(bpp == 24 || bpp == 32){  //if high color img, dither to 256 colors
		FIBITMAP *dithered_bitmap = FreeImage_ColorQuantize(bitmap, FIQ_WUQUANT);	
		//FIBITMAP *dithered_bitmap = FreeImage_ColorQuantizeEx(bitmap, FIQ_WUQUANT,256,0,NULL);	
		//FIQ_WUQUANT
		//FIQ_NNQUANT
		//FIQ_LFPQUANT
		//FIBITMAP *dithered_bitmap = FreeImage_ConvertTo8Bits(bitmap);
		if(dithered_bitmap == NULL){

#ifndef DISABLE_TRACE
			if(srcTrace){
				fprintf(stderr,"[readIMG_name] failed quiantize\n");

			}
#endif
			FreeImage_Unload(bitmap);
			FreeImage_DeInitialise();
			return NULL;


		}else{
			FreeImage_Unload(bitmap);
			bitmap = dithered_bitmap;
		}



	}	





	bpp = FreeImage_GetBPP(bitmap);
#ifndef DISABLE_TRACE
	if(srcTrace){
		fprintf(stderr,"bpp after: %d\n",bpp);
 	int numColors = FreeImage_GetColorsUsed(bitmap);
	
		fprintf(stderr,"numColors after: %d\n",numColors);
	}
#endif


	
	RGBQUAD *pal = FreeImage_GetPalette(bitmap);


	if(FreeImage_HasBackgroundColor(bitmap)){
		*bg = 1;
#ifndef DISABLE_TRACE
	if(srcTrace){
		fprintf(stderr,"Image has background color\n");
	}
#endif

	}
	else{
		*bg = -1;
	}	

	if(FreeImage_IsTransparent(bitmap)){
		transparent_index = FreeImage_GetTransparentIndex(bitmap);
#ifndef DISABLE_TRACE
	if(srcTrace){
		fprintf(stderr,"Image is transparent\n");
		fprintf(stderr,"index: %d \n",transparent_index);
	}
#endif	
	}



	if(pal){
		if(transparent_index == -1){  // no transparency
			int nColors = FreeImage_GetColorsUsed(bitmap);
			for(int i = 0; i < (nColors -1); i++){   //パレットのコピー
				colors[i].red = pal[i].rgbRed << 8; //256倍している
				colors[i].green = pal[i].rgbGreen << 8;
				colors[i].blue = pal[i].rgbBlue << 8;
				colors[i].pixel = i;
				colors[i].flags = DoRed|DoGreen|DoBlue;
			}
		}
		else{	
			extern Widget view;
			unsigned long bg_pixel;
			XColor bgcolr;
			XtVaGetValues(view, XtNbackground, &bg_pixel, NULL);
			bgcolr.pixel = bg_pixel;
			XQueryColor(XtDisplay(view),DefaultColormap(XtDisplay(view),0),&bgcolr);
			int nColors = FreeImage_GetColorsUsed(bitmap);
			for(int i = 0; i < (nColors -1); i++){  //パレットのコピー
				if(i == transparent_index){  // 背景色	
					colors[i].red = bgcolr.red;
					colors[i].green = bgcolr.green;
					colors[i].blue = bgcolr.blue << 8;
					colors[i].pixel = i;
					colors[i].flags = DoRed|DoGreen|DoBlue;

				}
				else{
					colors[i].red = pal[i].rgbRed << 8; //256倍している
					colors[i].green = pal[i].rgbGreen << 8;
					colors[i].blue = pal[i].rgbBlue << 8;
					colors[i].pixel = i;
					colors[i].flags = DoRed|DoGreen|DoBlue;
				}
			}


		}
	}
	else{  //ここまで来てもパレットがない= gray scale
		for(int i = 0; i < 255; i++){   //パレットのコピー
			colors[i].red = i << 8;
			colors[i].green = i << 8;
			colors[i].blue = i << 8;
			colors[i].pixel = i;
			colors[i].flags = DoRed|DoGreen|DoBlue;
		}

	}


	pixmap = (unsigned char *)malloc((*w)*(*h)*sizeof(unsigned char));
	
	unsigned char *bits = (unsigned char *)FreeImage_GetBits(bitmap);
	int size = (*w)*(*h)*sizeof(unsigned char);
	
		for(int i=0;i<(*h);i++){
			for(int j = 0;j<(*w);j++){
				pixmap[i*(*w)+j] = bits[(*h - i -1)*(*w) + j];
			}

		}

	// 画像の解放
	FreeImage_Unload(bitmap);

	// FreeImageの終了
	FreeImage_DeInitialise();

	return  pixmap;
}
