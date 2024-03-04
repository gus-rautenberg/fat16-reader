#include <stdio.h>
#include <stdlib.h>
// gcc -O2 fat16reader.cpp -o main
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

} fat_BR;

typedef struct fat_RD
{
    unsigned char file_name[11];
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

int main()
{
    FILE *fp;
    fat_BR boot_record;

    fp = fopen("../Data/fat16_1sectorpercluster.img", "rb");
    fseek(fp, 0, SEEK_SET);
    fread(&boot_record, sizeof(fat_BR), 1, fp);

    int root_begin = boot_record.reserved_sector_count + (boot_record.table_size_16 * boot_record.table_count);
    int root_start_bytes = root_begin * boot_record.bytes_per_sector;

    printf("Bytes per sector %hd \n", boot_record.bytes_per_sector);
    printf("Sector per cluster %x \n", boot_record.sectors_per_cluster);
    printf("Reserved Sector to BR %x \n", boot_record.reserved_sector_count);
    printf("Fat16 Count %x \n", boot_record.table_count);
    printf("Root Dir Entries %d \n", boot_record.root_entry_count);
    printf("Sectors per Fat %d \n", boot_record.table_size_16);
    printf("Sector per track %d \n", boot_record.sectors_per_track);

    printf("\n");
    printf("---------------------- Root Directory -------------------------");
    printf("\n");
    fat_RD root_directory;
    printf("Root Dir Sector Start %d \n", root_begin);
    printf("Root Start Byters %d \n", root_start_bytes);
    printf("\n");

    // fseek(fp, 159424, SEEK_SET);
    // fread(&root_directory, sizeof(fat_RD), 1, fp);
    // printf("Name %s \n", root_directory.file_name);
    // printf("atributes: %02X \n", root_directory.file_attributes);
    // printf("First Cluster: %04X \n", root_directory.low_16_bits_fc);
    // printf("Size %u\n", root_directory.file_size);
    fseek(fp, root_start_bytes, SEEK_SET);
    fread(&root_directory, sizeof(fat_RD), 1, fp);

    // printf("Root Dir Sector Start %d \n", root_begin);

    int count = 0;

    for (int i = 0; i < boot_record.root_entry_count; i++)
    {
        if (!(root_directory.file_attributes == 0x0F) && !(root_directory.file_attributes == 0))
        {
            printf("Name %s \n", root_directory.file_name);
            printf("atributes: %02X ", root_directory.file_attributes);
            root_directory.file_attributes == 0x20 ? printf("<-- FILE\n") : printf("<-- DIRECTORY\n");
            printf("First Cluster: %04X \n", root_directory.low_16_bits_fc);
            printf("Size %u\n", root_directory.file_size);
            printf("\n");
        }

        if (root_directory.file_attributes == 0x20 && root_directory.low_16_bits_fc > 0x00)
        {
            
        }
        count += 32;
        fseek(fp, root_start_bytes + count, SEEK_SET);
        fread(&root_directory, sizeof(fat_RD), 1, fp);
    }

    return 0;
}
