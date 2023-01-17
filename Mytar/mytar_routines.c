#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mytar.h"

extern char *use;

/** Copy nBytes bytes from the origin file to the destination file.
 *
 * origin: pointer to the FILE descriptor associated with the origin file
 * destination:  pointer to the FILE descriptor associated with the destination file
 * nBytes: number of bytes to copy
 *
 * Returns the number of bytes actually copied or -1 if an error occured.
 */
//Copia nBytes de origin a destination
int copynFile(FILE * origin, FILE * destination, int nBytes)
{
	char* c = (char*) malloc(sizeof(char) * nBytes);	//crea y reserva el espacio en el array c que se empleara para copiar nBytes desde origin
	
	int num = fread(c, sizeof(char), nBytes, origin);	//lee de origin nBytes elementos de tamano sizeof(char) y los copia en c. num es el numero de bytes leidos
	
	fwrite(c, sizeof(char), num, destination);	//escribe num elementos de tamano sizeof(char) desde c a destination
	
	free(c);
	
	return num;	//devolvemos el numero de bytes copiados
}

/** Loads a string from a file.
 *
 * file: pointer to the FILE descriptor 
 * 
 * The loadstr() function must allocate memory from the heap to store 
 * the contents of the string read from the FILE. 
 * Once the string has been properly built in memory, the function returns
 * the starting address of the string (pointer returned by malloc()) 
 * 
 * Returns: !=NULL if success, NULL if error
 */
//Lee un string del fichero file
char* loadstr(FILE * file)
{
	int count = 0;	//contador para indicar el tamano que ocupa el string

	while(getc(file) != '\0')	//calcula el tamano del string leyendo caracter a caracter y aumentando el contador
	{
		count++;
	}

	fseek(file, -(count + 1), SEEK_CUR);	//una vez se conoce el tamano, situa el puntero al inicio del string

	char* c = (char*) malloc(count * sizeof(char));	//crea y reserva espacio al array c para poder copiar el string

	fread(c, sizeof(char), count + 1, file);	//lee el string y lo guarda en el array c

	return c;	//devuelve el string
}

/** Read tarball header and store it in memory.
 *
 * tarFile: pointer to the tarball's FILE descriptor 
 * nFiles: output parameter. Used to return the number 
 * of files stored in the tarball archive (first 4 bytes of the header).
 *
 * On success it returns the starting memory address of an array that stores 
 * the (name,size) pairs read from the tar file. Upon failure, the function returns NULL.
 */
//Lee el encabezado del fichero tarFile
stHeaderEntry* readHeader(FILE * tarFile, int *nFiles)
{
	fread(nFiles, sizeof(int), 1, tarFile);	//lee el numero de ficheros que encontraremos en el mtar y lo guarda en nFiles (como es por referencia, la actualiza)

	stHeaderEntry* cabecera = (stHeaderEntry*) malloc((*nFiles) * sizeof(stHeaderEntry));	//crea la cabecera de tipo stHeaderEntry y reserva una entrada para cada fichero

	for(int i = 0; i < *nFiles; i++)
	{
		cabecera[i].name = loadstr(tarFile);	//lee directamente del fichero mtar y guarda la info como nombre
		fread(&(cabecera[i].size), sizeof(int), 1, tarFile);	//lo mismo para leer el tamano
	}

	return cabecera;	//devuelve la cabecera, que incluira los nombres y tamanos de todos los ficheros
}

/** Creates a tarball archive 
 *
 * nfiles: number of files to be stored in the tarball
 * filenames: array with the path names of the files to be included in the tarball
 * tarname: name of the tarball archive
 * 
 * On success, it returns EXIT_SUCCESS; upon error it returns EXIT_FAILURE. 
 * (macros defined in stdlib.h).
 *
 * HINTS: First reserve room in the file to store the tarball header.
 * Move the file's position indicator to the data section (skip the header)
 * and dump the contents of the source files (one by one) in the tarball archive. 
 * At the same time, build the representation of the tarball header in memory.
 * Finally, rewind the file's position indicator, write the number of files as well as 
 * the (file name,file size) pairs in the tar archive.
 *
 * Important reminder: to calculate the room needed for the header, a simple sizeof 
 * of stHeaderEntry will not work. Bear in mind that, on disk, file names found in (name,size) 
 * pairs occupy strlen(name)+1 bytes.
 *
 */
//Crea el fichero tar juntando varios ficheros dados en fileNames[]
int createTar(int nFiles, char *fileNames[], char tarName[])
{
	FILE * f = fopen(tarName, "w");	//abre el mtar para escribir la info

	int tamCabecera = sizeof(int);	//crea el tamano de la cabecera -> numero de ficheros, nombre y tamano de todos los ficheros
	for(int i = 0; i < nFiles; i++)
	{
		tamCabecera += strlen(fileNames[i]) + 1 + sizeof(int);
	}

	fseek(f, tamCabecera, SEEK_SET);	//avanza el puntero para copiar primero la info de los ficheros

	int* fileSizes = (int*) malloc(nFiles * sizeof(int));	//crea y reserva espacio para el numero que indica el tamano de cada fichero
	
	for(int i = 0; i < nFiles; i++)
	{
		FILE * p = fopen(fileNames[i], "r");
		
		int tamCopiado = copynFile(p, f, 200);	//copia la info del fichero p al mtar de 200 en 200 bytes y guarda el tamano copiado real

		fileSizes[i] = 0;

		while(tamCopiado != 0)	//sigue copiando y actualizando el tamano copiado del fichero en el array fileSizes
		{
			fileSizes[i] += tamCopiado;
			tamCopiado = copynFile(p, f, 200);
		}

		fclose(p);
	}
	
	fseek(f, 0, SEEK_SET);	//coloca el puntero al inicio para escribir la info en el encabezado del mtar

	fwrite(&nFiles, sizeof(int), 1, f);	//indica el numero de ficheros al inicio del encabezado

	for(int i = 0; i < nFiles; i++)	//escribe el nombre y tamano para cada fichero
	{
		fwrite(fileNames[i], sizeof(char), strlen(fileNames[i]) + 1, f);
		fwrite(&fileSizes[i], sizeof(int), 1, f);
	}

	fclose(f);
	free(fileSizes);

	printf("Fichero .mtar creado\n");

	return EXIT_SUCCESS;
}

/** Extract files stored in a tarball archive
 *
 * tarName: tarball's pathname
 *
 * On success, it returns EXIT_SUCCESS; upon error it returns EXIT_FAILURE. 
 * (macros defined in stdlib.h).
 *
 * HINTS: First load the tarball's header into memory.
 * After reading the header, the file position indicator will be located at the 
 * tarball's data section. By using information from the 
 * header --number of files and (file name, file size) pairs--, extract files 
 * stored in the data section of the tarball.
 *
 */
//Extrae todos los ficheros que forman el fichero tar
int extractTar(char tarName[])
{
	FILE * f = fopen(tarName, "r");	//abre el fichero mtar para leer la info

	int nFiles;
	stHeaderEntry* cabecera = readHeader(f, &nFiles);	//lee la cabecera y el numero de ficheros que componen el mtar

	for(int i = 0; i < nFiles; i++)	//abre el fichero con el nombre que le corresponde y copia tantos bytes como ocupa
	{
		FILE * p = fopen(cabecera[i].name, "w");
		copynFile(f, p, cabecera[i].size);
		fclose(p);
	}
	fclose(f);

	printf("Fichero .mtar extraído\n");

	return EXIT_SUCCESS;
}