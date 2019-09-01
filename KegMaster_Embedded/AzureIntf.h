#pragma once
/*----- Literal Constants -----*/

/*----- Types -----*/

/*----- Functions -----*/
int Azure_Init();

/* Keg Data - One row per keg, updated until tapped out */
int Azure_FtchKegData(int kegId); /* Get Keg Data from Azure SQL	*/
int Azure_UpdtKegData(int kegId); /* Update Local Keg Data			*/
int Azure_SendKegData(int kegId); /* Update Remote Keg Data (SQL)	*/ 

/* Keg Event Data - One row per action, persists */
int Azure_FtchKegEventData(int kegId); /* Get Keg Data from Azure SQL	*/
int Azure_UpdtKegEventData(int kegId); /* Update Local Keg Data			*/
int Azure_SendKegEventData(int kegId); /* Update Remote Keg Data (SQL)	*/

/* User Data - One row per user, updated until remove */
int Azure_FtchUserData(int kegId); /* Get Keg Data from Azure SQL	*/
int Azure_UpdtUserData(int kegId); /* Update Local Keg Data			*/
int Azure_SendUserData(int kegId); /* Update Remote Keg Data (SQL)	*/
