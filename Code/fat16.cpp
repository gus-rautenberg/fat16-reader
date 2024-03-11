#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <unordered_map>

// g++ -O2 fat16reader.cpp -o main
#pragma pack(push, 1)

typedef struct fat_BS
{
    unsigned char bootjmp[3];
    unsigned char oem_name[8];
    unsigned short bytes_per_sector;
    unsigned char sectors_per_cluster;
    unsigned short reserved_sector_count;
    unsigned char table_count;
    unsigned short root_entry_count;
    unsigned short total_sectors_16;
    unsigned char media_type;
    unsigned short table_size_16;
    unsigned short sectors_per_track;
    unsigned short head_side_count;
    unsigned int hidden_sector_count;
    unsigned int total_sectors_32;

    // this will be cast to it's specific type once the driver actually knows what type of FAT this is.
    unsigned char extended_section[54];

} __attribute__((packed)) fat_BR;

typedef struct fat_RD
{
    unsigned char file_name[8];
    unsigned char file_extension[3];
    unsigned char file_attributes;
    unsigned char windowsNT;
    unsigned char creation_time;
    unsigned short time_file_created;
    unsigned short date_file_created;
    unsigned short last_access;
    unsigned short high_16_bits_fc;
    unsigned short last_modification_time;
    unsigned short last_modification_date;
    unsigned short low_16_bits_fc;
    unsigned int file_size;

} fat_RD;

#pragma pack(pop)

void readingFat(FILE *file, fat_BR *boot_record, fat_RD *root_directory, std::vector<int> *clustersFat){
    int fat_start = boot_record->bytes_per_sector * boot_record->reserved_sector_count;
    int firstClusterStart = fat_start + (root_directory->low_16_bits_fc) * 2;
    clustersFat->push_back(root_directory->low_16_bits_fc);
    fseek(file, firstClusterStart, SEEK_SET);
    unsigned short nextCluster = 0;

    while (fread(&nextCluster, sizeof(nextCluster), 1, file) == 1 && nextCluster != 0xFFFF && nextCluster != 0)
    {
        clustersFat->push_back(nextCluster);
        fseek(file, (fat_start + (nextCluster) * 2), SEEK_SET);
    }
}

void readingData(FILE *file, fat_BR *boot_record, fat_RD *root_directory, std::vector<int> *clustersFat, int dataStart) {
    printf("------------------- File Content --------------------\n");
    int clusterSize = boot_record->sectors_per_cluster * boot_record->bytes_per_sector;
    int fileSize = root_directory->file_size;
    for (int j = 0; j < clustersFat->size(); j++)
    {  
        unsigned char data[clusterSize];
        fseek(file, ((((*clustersFat)[j] - 2) * boot_record->sectors_per_cluster) + dataStart) * boot_record->bytes_per_sector, SEEK_SET);
        if (clusterSize > fileSize)
        {
            fread(&data, fileSize, 1, file);
            for (int k = 0; k < fileSize; k++)
            {
                printf("%c", data[k]);
            }
        }
        else
        {
            fread(&data, clusterSize, 1, file);
            for (int k = 0; k < clusterSize; k++)
            {
                printf("%c", data[k]);
            }
            fileSize -= clusterSize;
        }
    }
    printf("\n");
    printf("-----------------------------------------------------------------\n");
}
int main()
{
    FILE *fp;
    fat_BR boot_record;
    fat_RD root_directory;

    // fp = fopen("../Data/fat16_1sectorpercluster.img", "rb");
    fp = fopen("../Data/fat16_4sectorpercluster.img", "rb");
    // fp = fopen("../Data/floppyext2.img", "rb");
    // fp = fopen("../Data/test.img", "rb");

    fseek(fp, 0, SEEK_SET);
    fread(&boot_record, sizeof(fat_BR), 1, fp);

    printf("========================================================\n");
    printf("||                    Boot Record                     ||\n");
    printf("========================================================\n");
    printf("\n");

    printf("Bytes per sector: %hd \n", boot_record.bytes_per_sector);
    printf("Sector per cluster: %x \n", boot_record.sectors_per_cluster);
    printf("Reserved Sector to BR: %x \n", boot_record.reserved_sector_count);
    printf("Fat16 Count: %x \n", boot_record.table_count);
    printf("Root Dir Entries: %d \n", boot_record.root_entry_count);
    printf("Sectors per Fat: %d \n", boot_record.table_size_16);
    printf("Sector per track: %d \n", boot_record.sectors_per_track);



    int root_begin = boot_record.reserved_sector_count + (boot_record.table_size_16 * boot_record.table_count);
    int root_start_bytes = root_begin * boot_record.bytes_per_sector;
    int root_dir_sector = ((boot_record.root_entry_count * 32) + (boot_record.bytes_per_sector - 1)) / boot_record.bytes_per_sector;
    int dataStart = (root_begin + root_dir_sector);
    printf("\n");
    printf("========================================================\n");
    printf("||                   Root Directory                   ||\n");
    printf("========================================================\n");
    printf("\n");
    


    fseek(fp, root_start_bytes, SEEK_SET);
    fread(&root_directory, sizeof(fat_RD), 1, fp);
    std::vector<fat_RD> files;

    int count = 0;
    int fileCount = 0;
    for (int i = 0; i < boot_record.root_entry_count; i++)
    {
        if (!(root_directory.file_attributes == 0x0F) && !(root_directory.file_attributes == 0) && !(root_directory.file_name[0] == 0xE5))
        {
            printf("----------------------------------------------\n");
            printf("Name: ");
            for(int c = 0; c < sizeof(root_directory.file_name); c++){
                printf("%c", root_directory.file_name[c]);
            }
            printf("\n");
            printf("Extension: ");
            for(int c = 0; c < sizeof(root_directory.file_extension); c++){
                printf("%c", root_directory.file_extension[c]);
            }
            printf("\n");
            printf("atributes: %02X ", root_directory.file_attributes);
            root_directory.file_attributes == 0x20 ? printf("<-- FILE\n") : printf("<-- DIRECTORY\n");
            printf("First Cluster: %04X \n", root_directory.low_16_bits_fc);
            printf("Size %u\n", root_directory.file_size);
            if (root_directory.file_attributes == 0x20 && root_directory.low_16_bits_fc > 0x00)
            {

                files.push_back(root_directory);
                fileCount++;
            }
        }
        count += 32;
        fseek(fp, root_start_bytes + count, SEEK_SET);
        fread(&root_directory, sizeof(fat_RD), 1, fp);
    }
    printf("----------------------------------------------\n");
    printf("\n");
    int option = -1;
    while(1){
        std::vector<int> clustersFat;

        printf("========================================================\n");
        printf("||                    File's Menu                     ||\n");
        printf("========================================================\n");
        for(int j = 0; j<fileCount; j++){
            printf("File : %d \n", j);
            printf("Name: ");
            for(int c = 0; c < sizeof(files[j].file_name); c++){
                printf("%c", files[j].file_name[c]);
            }
            printf("\n");
            printf("Extension: ");
            for(int c = 0; c < sizeof(files[j].file_extension); c++){
                printf("%c", files[j].file_extension[c]);
            }
            printf("\n");
            printf("----------------------------------------------\n");

        }
        printf("Choose File (insert number):  ");
        scanf("%d", &option);
        if(option < 0){
            break;
        } else if( option >= fileCount){
            printf("Not a valid option\n");
            continue;
        }
        readingFat(fp, &boot_record, &files[option], &clustersFat);
        readingData(fp, &boot_record, &files[option], &clustersFat, dataStart);
        
    }
    return 0;
}