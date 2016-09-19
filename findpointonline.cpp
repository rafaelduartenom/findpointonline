#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libpq-fe.h"
#include <shapefile.h>
double *find_near(PGconn *conn,double x,double y,double z,float tole, char *tablename, char *epsgcode, float prec);
int count_shape_points(char *shape_name);
void find_file_shp(char *name,char *name_metadata);
void set_output_metadata(char *name);
double *get_coordinate_point(int point_id, char *name_of_shp);



int main(int argc, char* argv[])
{
FILE * Temp_File;
    if ( argc < 11 ) /* argc should be 2 for correct execution */
    {
        /* We print argv[0] assuming it is the program name */
        printf( "Correct arguments: %s host_postgis port dbname user password tolerance tablename shapefilewithoutdotshp EPSG:CODE precision output.csv", argv[0] );
    }
Temp_File = fopen (argv[11],"w");
//verifica se o arquivo shape esta abrindo
char nomeX_metadado[150];
char nomefolha[30];
char nomeshp[100];
int number_of_points;
strcpy (nomefolha,argv[8]);
strcpy (nomeX_metadado,nomefolha);
strcpy (nomeshp,nomefolha);
set_output_metadata(nomeX_metadado);
find_file_shp(nomeshp,nomeX_metadado);
number_of_points = count_shape_points(nomeshp);
for (int b = 0; b < number_of_points; b++)
{

double *point;


point = get_coordinate_point(b,nomeshp);
// Cria uma conexão com um banco PostgreSQL

PGconn *conn = PQsetdbLogin(
argv[1], // host
argv[2], // port
NULL, // options
NULL, // tty
argv[3], // dbName
argv[4], // login
argv[5] // password
);
printf("\n");
printf("Point Shapefile: ");
fprintf(Temp_File,"Point Shapefile;");
for (int i = 0; i < 3; i++)
{
printf("%f ",point[i]);
char shpoint[16];
char * dot1;
sprintf(shpoint,"%f", point[i]);
dot1 = strchr(shpoint,'.');
if (dot1){*dot1 = ',';}
fprintf(Temp_File,"%s;",shpoint);
}
double *result;
result = find_near(conn,point[0], point[1],point[2],atof(argv[6]),argv[7],argv[9],atof(argv[10]));
printf("Point Near: ");
fprintf(Temp_File,"Point Near;");
for (int i = 0; i < 3; i++)
{
printf("%f ",result[i]);
char resultpoint[16];
char * dot2;
sprintf(resultpoint,"%f", result[i]);
dot2 = strchr(resultpoint,'.');
if (dot2){*dot2 = ',';}
fprintf(Temp_File,"%s;",resultpoint);
}
printf("\n");
fprintf(Temp_File,"\n");
// Fecha a conexão com o banco
PQfinish(conn);
}
fclose (Temp_File);
return 0;
}


double *find_near(PGconn *conn,double x,double y,double z,float tole, char *tablename, char *epsgcode,float prec)
{
float i=prec;
static double r[]={0,0,0,0};
while(i<=tole) {
char output_command[400];
sprintf(output_command,"SELECT ST_X(geom), ST_Y(geom), ST_Z(geom) FROM (SELECT ST_3DClosestPoint(l.geom, p.geometry) AS geom FROM ST_SetSRID(ST_MakePoint(%f,%f,%f),%s) As p , %s l WHERE ST_Intersects(ST_Buffer((ST_SetSRID(ST_MakePoint(%f,%f,%f),%s)),%f),l.geom)) AS point;",x,y,z,epsgcode,tablename,x,y,z,epsgcode,i);
// Faz a consulta ao banco
PGresult *res = PQexecParams(conn, output_command,0, NULL, NULL, NULL, NULL, 0);

// Verifica se a consulta foi valida
if (PQresultStatus(res) != PGRES_TUPLES_OK)
{
printf(PQerrorMessage(conn));
PQclear(res);
PQfinish(conn);
exit (0);
}

// Obtém o número de registros
int nRegistros = PQntuples(res);

if (nRegistros != 0) {

// Percorre todas as colunas
for (int j = 0; j < 3; j++)
{
char result[15];
sprintf(result,"%-15s",PQgetvalue(res, 0, j));
r[j]= atof(result);
}
// Fecha o acesso aos registros
PQclear(res);
if (r[0]) return r;
}
i=i+prec;
}
printf("oi");
if (i>tole){memset(r, 0, sizeof r);}
return r;
}

int count_shape_points(char *shape_name)
{
    int entidades;
    SHPHandle arquivo_shp;
    arquivo_shp=SHPOpen(shape_name, "r");
    SHPGetInfo(arquivo_shp,&entidades,NULL,NULL,NULL);
    return entidades;
}


void find_file_shp(char *name,char *name_metadata)
{
    FILE * metadados;
    metadados = fopen (name_metadata,"a");
    char shp_plus_in_folder[100];
    char shp_plus[30];
    SHPHandle arquivo_shp;
    strcpy (shp_plus,name);
    strcat (shp_plus,".shp");
    sprintf(shp_plus_in_folder,"%s",shp_plus);//Para o IGC
    //abre o shp e verifica se o arquivo existe
    arquivo_shp=SHPOpen(shp_plus_in_folder, "r");
        if (arquivo_shp==NULL){
                           printf("Nao consegui abrir %s na pasta SHP (IGC)\n",shp_plus_in_folder);
                           fprintf(metadados,"Nao consegui abrir %s na pasta SHP (IGC)\n",shp_plus_in_folder);
                           exit(0);
                           }
//fim abre shp
strcpy (name,shp_plus_in_folder);
SHPClose(arquivo_shp);
fclose(metadados);
}


void set_output_metadata(char *name)
{
    FILE *file_output;
    char output_plus[80];
    char metadata_in_folder[50];
    char folder_metadata[30];
    char folder_plus_file[150];
    strcpy (output_plus,name);
    strcpy (folder_metadata,name);
    strcpy (metadata_in_folder,name);
    strcat (output_plus,"_erros_pontos_metadados.xml");
    strcat (metadata_in_folder,"_erros_pontos_metadados.xml");
    sprintf(folder_plus_file,"..\\SHP\\%s",output_plus);//Para o IGC
    //verifica se o shp pode ser gravado
    file_output=fopen (folder_plus_file, "w+");

        if (file_output==NULL){
                           file_output=fopen (metadata_in_folder, "w+");
                           strcpy (name,metadata_in_folder);
                           if (file_output==NULL)
                           {
                           printf("Erro ao gravar o metadados da folha %s verifique se a pasta de saida existe",name);
                           getchar();
                           exit(1);}
        }
        else {strcpy (name,folder_plus_file);}
}


double *get_coordinate_point(int point_id, char *name_of_shp)
{
    SHPObject *point;
    SHPHandle arquivo_shp;
    static double p[4];
    arquivo_shp=SHPOpen(name_of_shp, "r");
    point = SHPReadObject(arquivo_shp,point_id);
       p[0] = point->padfX[0];
       p[1] = point->padfY[0];
       p[2] = point->padfZ[0];
SHPClose(arquivo_shp);
return p;
}
