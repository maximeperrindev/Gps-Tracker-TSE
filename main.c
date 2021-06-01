/* ========================================
 *
 * Télécom Saint-Étienne - FISE 1
 * Projet d'application électronique 2021 - TRACKER GPS
 * Maxime Perrin - Agathe Chabrol
 *
 * ========================================
*/
#include "project.h"
#include <stdio.h>
#define GO_LATITUDE     1u
#define GO_LONGITUDE    2u

/* DÉCLARATION DE VARIABLES */

uint8 State=GO_LATITUDE;

uint8 commaNum=0;   //Indice de la virugle
char fixFlag='0';

char latitude[9];   //latitude
char *latitudePtr = latitude; //Pointeur associé à la latitude

char longitude[10]; //longitude        
char *longitudePtr = longitude; //Pointeur associé à la longitude

uint8 commaLoc[14]={0}; //Tableau qui stocker les indices des virgules

char colon = ':'; //Variable ':'
uint8 colon_flag = 0; //Détection du ':'

char dataByte = 0x00; //Données du GPS
char rxBuf[1024]; //Buffer de stockage de trame GPS
char *rxBufPtr = rxBuf; //Pointer associé au buffer
uint8 end_flag = 0; //Indicateur de fin de trame

/* DÉCLARATION DE FONCTION */

void GetGPSMessage(uint8 State); //Déclaration de la fonction qui récupère la latitude ou la longitude

int main(void)
{
    CyGlobalIntEnable; /* Activation des interruptions globales. */
    /* Initialisation des clocks et UART */
    UART_GPS_Start();
    Clock_GPS_Start();
    UART_PC_Start();
    Clock_PC_Start();
    
    /* Boucle infinie qui va faire tourner notre programme */
    for(;;)
    {   
        dataByte = UART_GPS_GetChar(); // Récupération des coordonées via l'UART (Norme NMEA 0183)
        UART_PC_PutChar(dataByte);
        if (dataByte == 0x24) //Si la donnée est complète
		{
            for(;;) //On parcourt toute la chaîne de caractère correspondant à la position
			{
				switch (dataByte)
				{
					case 0x00: // Si il n'y a rien
					{
						break;
					}
					
					case 0x24: //Si cela commence par '$' (commande normée NMEA 0183)
					{
						*rxBufPtr = dataByte;
						rxBufPtr++;
						break;
					}
					
					case 0x0A: //Si le caractère est '<LF>' : trame terminée
					{
						*rxBufPtr = dataByte;
						*(rxBufPtr + 1) = 0x00;
						end_flag = 1;
						break;
					}
					
					default:
					{
						*rxBufPtr = dataByte;
						rxBufPtr++;
						break;
					}
				}
                dataByte = UART_GPS_GetChar();
				
				if (end_flag == 1) //Si la trame est terminée
				{
					end_flag = 0;
					rxBufPtr = rxBuf;
				
                 
					if ( *(rxBufPtr + 4) == 'G' ) //Trame $GPGGA détectée (La trame GGA est très courante car elle fait partie de celles qui sont utilisées pour connaître la position courante du récepteur GPS.)
					{   
                     	commaNum=0;
                        uint8 i=0;
						for(i=0;i<100;i++)//On récupère l'indice des virgules et du fixFlag
						{
							if(*(rxBuf+i)==',') //Si c'est une virgule
							{	
								*(commaLoc+commaNum)=i; //On récupère l'indice de la virgule
								commaNum++;											      
								if(commaNum == 6) //Si c'est un fixFlag
								{
									fixFlag=*(rxBuf+i+1); //On récupère le fixFlag
								}
                                
							}
								
						}
                         	
						if((fixFlag == '1')||(fixFlag == '2'))// 0=Fix indisponible/incorrect , 1= Mode GPS SPS, Fix valide, 2= GPS Différentiel, Mode SPS, Fix Valide, 6= Mode Estimé (Dead Reckoning) 
						{	
                        	GetGPSMessage(State);
						}
                       
                      /* Construction de la trame série à envoyer au backend */
                        
                      GetGPSMessage(GO_LATITUDE);
                      UART_PC_PutString(",");
                      GetGPSMessage(GO_LONGITUDE);
                      UART_PC_PutString(",");
                      UART_PC_PutString("26"); //Température simulée			
                      UART_PC_PutChar('\n'); //Saut de ligne OBLIGATOIRE pour fonctionner avec le parser du backend node.js
                      
                    
                    /* Sortie sous la forme : lat,lon,degrés*/
           
					}
					
				}
            }
        }
        
    }
        

}

void GetGPSMessage(uint8 State) //Fonction qui retourne la latitude, la longitude, ou le TIME du GPS
{
        uint8 latloc=0;
		uint8 loloc=0;
        
        switch( State )
        {
        case GO_LATITUDE: 
            
            memset(latitude,0,9);  //Reset de mémoire
									
            uint8 la=0;
            /* commaLoc est l'adresse de la première virgule. Or, la latitude se situe après la deuxième virgule d'où le *(commaLoc+1)+1*/
			for(la=(*(commaLoc+1)+1);la<*(commaLoc+2);la++) //On récupère la latitude
				{
					*(latitudePtr+latloc) = *(rxBufPtr+la);//On ajoute chaque caractère
					latloc++;
				}
            UART_PC_PutChar(' ');				
			UART_PC_PutString(latitude);
            
			memset(latitude,0,9);  //Reset de mémoire
            
          
            
            break;
            
        case GO_LONGITUDE :
             
            memset(longitude,0,10);  //Reset mémoire
            uint8 lo=0;
            /* De même, la longitude se situe après la 4ème virgule d'où le *(commaLoc+3)+1*/
			for(lo=(*(commaLoc+3)+1);lo<*(commaLoc+4);lo++)	//On prend la Longtitude
				{
				*(longitudePtr+loloc) = *(rxBufPtr+lo); //On ajoute chaque caractère
				loloc++;
				}
		    
			
			UART_PC_PutString(longitude);	
			memset(longitude,0,10);  //Reset mémoire
         
            break;
            default:
                break;
        
        }

						
                       
}                    
/* FIN DU CODE */
