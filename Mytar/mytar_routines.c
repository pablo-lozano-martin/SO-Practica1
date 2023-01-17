// Practica 1. Pablo Lozano, Gonzalo Contreras y Pablo Pezo


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
int
copynFile(FILE * origin, FILE * destination, int nBytes)
{
	// Complete the function
	int i = 0;

	int c = getc(origin);

	while (i < nBytes && c != EOF)
	{
		putc(c, destination);
		c = getc(origin);

		i++;
	}

	return i;
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
char* loadstr(FILE * file)
{
	int i = 0;
	int c = getc(file);

	while (c != '\0' && c != EOF)
	{
		i++;
		c = getc(file);
	}

	char* nombre = (char *) malloc(i);

	fseek(file, -i, SEEK_CUR);
	fread(nombre, 1, i, file);

	return nombre;
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
stHeaderEntry* readHeader(FILE *tarFile, int *nFiles)
{
	fread(nFiles, sizeof (int), 1, tarFile);

	stHeaderEntry* cabecera = (stHeaderEntry*) malloc((*nFiles) * (sizeof (stHeaderEntry)));

	for(int i = 0; i < *nFiles; i++)
	{
		cabecera[i].name = loadstr(tarFile);
		fread(&cabecera[i].size, sizeof(cabecera[i].size), 1, tarFile);
	}

	return cabecera;
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
int createTar(int nFiles, char *fileNames[], char tarName[])
{
	FILE *in, *tar;	
	unsigned int tamcabecera = sizeof(int);

	stHeaderEntry *cabecera = malloc(sizeof (stHeaderEntry) * nFiles);

	tar = fopen(tarName, "wx");

	for(int i = 0; i < nFiles; i++)
	{
		int namesize = strlen(fileNames[i]) + 1;

		cabecera[i].name = (char *) malloc(namesize);
		strcpy(cabecera[i].name, fileNames[i]);

		tamcabecera += namesize + sizeof(cabecera->size);
	}

	fseek(tar, tamcabecera, SEEK_SET);

	for(int i = 0; i < nFiles; i++)
	{
		in = fopen(fileNames[i], "r");

		cabecera[i].size = copynFile(in, tar, INT_MAX);
		fclose(in);
	}

	fseek(tar, 0, SEEK_SET);

	fwrite(&nFiles, sizeof(int), 1, tar);

	for(int i = 0; i < nFiles; i++)
	{
		fwrite(cabecera[i].name, 1, strlen(cabecera[i].name) + 1, tar);
		
		fwrite(&cabecera[i].size, sizeof (cabecera[i].size), 1, tar);
	}

	fprintf(stdout, "Archivo creado con exito\n");

	for (int i = 0; i < nFiles; i++)
	{
		free(cabecera[i].name);
	}
	
	free(cabecera);
	fclose(tar);

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
int extractTar(char tarName[])
{
	FILE *out, *tar;
	int num;
	
	tar = fopen(tarName, "r");
	stHeaderEntry *cabecera = readHeader(tar, &num);

	for(int i = 0; i < num; i++)
	{
		out = fopen(cabecera[i].name, "w");
		copynFile(tar, out, cabecera[i].size);

		fclose(out);
	}

	for(int i = 0; i < num; i++)
	{
		free(cabecera[i].name);
	}
	
	free(cabecera);
	fclose(tar);

	return EXIT_SUCCESS;
}
