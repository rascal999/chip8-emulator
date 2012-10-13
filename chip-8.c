#include <stdio.h>
#include <SDL.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BLOCK 10
#define WIDTH 640
#define HEIGHT 320
#define BPP 4
#define DEPTH 32

/* TODO

   - Struct for display
   - Reorganise UpdateGraphics func to correct area in file
   - Display API
*/

typedef struct {
   unsigned short opcode; /* One of 35 opcodes */
   unsigned char memory[4096]; /* 4K memory */
   unsigned char V[16]; /* 16 registers V0 .. V15 */
   unsigned short I; /* Index register */
   unsigned short pc; /* Program counter */
   unsigned char gfx[64*32]; /* Graphics */
   unsigned char delay_timer;
   unsigned char sound_timer;
   unsigned short stack[16]; /* Stacks stack0 .. stack15 */
   unsigned short sp;  /* Stack pointer */
   unsigned char key[16]; /* HEX based keypad (0x0-0xF) */
   int ROMfd; /* ROM fd */
   unsigned char ROM[4096]; /* Loaded ROM */
   int DrawFlag; /* Draw? */
} Chip8;

typedef struct {
   SDL_Surface *screen;
   SDL_Event event;
   int keypress;
   int x;
   int y;
   int c;
} Display;  

unsigned char chip8_fontset[80] =
{ 
   0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
   0x20, 0x60, 0x20, 0x20, 0x70, // 1
   0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
   0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
   0x90, 0x90, 0xF0, 0x10, 0x10, // 4
   0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
   0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
   0xF0, 0x10, 0x20, 0x40, 0x40, // 7
   0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
   0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
   0xF0, 0x90, 0xF0, 0x90, 0x90, // A
   0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
   0xF0, 0x80, 0x80, 0x80, 0xF0, // C
   0xE0, 0x90, 0x90, 0x90, 0xE0, // D
   0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
   0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

int exiterror(int err)
{
   switch(err)
   {
      case 2:
         printf("Error 1: Cannot open file\n");
         exit(2);
         break;
      case 3:
         printf("Error 3: Error reading from file\n");
         exit(3);
         break;
      case 20:
         printf("Error 20: Missing opcode\n");
         exit(20);
         break;
      default:
         printf("Error: Unknown error code\n");
         exit(1);
         break;
   }
}

/* Screen functions */
void setpixel(SDL_Surface *screen, int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
    Uint32 *pixmem32;
    Uint32 colour;  
 
    colour = SDL_MapRGB( screen->format, r, g, b );
  
    pixmem32 = (Uint32*) screen->pixels  + y + x;
    *pixmem32 = colour;
}

void DrawScreen(SDL_Surface* screen, int x, int y, int c)
{ 
    int ytimesw;
    int blocky;
    int blockx;
    x = x * BLOCK;
    y = y * BLOCK;
  
    if(SDL_MUSTLOCK(screen)) 
    {
        if(SDL_LockSurface(screen) < 0) return;
    }

    for(blocky=0;blocky<BLOCK;blocky++)
    {
       ytimesw = y*screen->pitch/BPP;
       for(blockx=0;blockx<BLOCK;blockx++)
       {
          setpixel(screen, blockx + x, (blocky*screen->pitch/BPP) + ytimesw, 255,255,255);
       }
    }

    if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
  
    SDL_Flip(screen); 
}

int InitialiseScreen()
{
    SDL_Surface *screen;
    /* SDL_Event event;
  
    int keypress = 0;
    int x = 63; 
    int y = 31;
    int c = 0; */
  
    if (SDL_Init(SDL_INIT_VIDEO) < 0 ) return 1;
   
    if (!(screen = SDL_SetVideoMode(WIDTH, HEIGHT, DEPTH, SDL_HWSURFACE)))
    {
        SDL_Quit();
        return 1;
    }
  
    /*while(1)
    {
       DrawScreen(screen,x,y,c);
    }*/
    /*while(!keypress) 
    {
         while(SDL_PollEvent(&event)) 
         {      
              switch (event.type) 
              {
                  case SDL_QUIT:
	              keypress = 1;
	              break;
                  case SDL_KEYDOWN:
                       keypress = 1;
                       break;
              }
         }
    }*/
    SDL_Quit();
  
    return 0;
}

int UpdateGraphics(Chip8 *chip8, Display *display)
{
   
   return 0;
}
/* END Screen functions */

int InitScreen(Display *display)
{
   return 0;
}

int InitCPU(Chip8 *chip8)
{
   int i;

   chip8->pc = 0x200;
   chip8->opcode = 0;
   chip8->I = 0;
   chip8->sp = 0;

   /* Clear registers V0-VF */
   for(i=0;i<16;i++)
   {
      chip8->V[i] = 0;
   }

   /* Clear display */
   for(i=0;i<64*32;i++)
   {
      chip8->gfx[i] = 0;
   }

   /* Clear stack */
   for(i=0;i<16;i++)
   {
      chip8->stack[i] = 0;
   }

   /* Clear memory */
   for(i=0;i<4096;i++)
   {
      chip8->memory[i] = 0;
   }

   /* Clear keypad */
   for(i=0;i<16;i++)
   {
      chip8->key[i] = 0;
   }

   /* Load fontset */
   for(i=0;i<80;i++)
   {
      chip8->memory[i] = chip8_fontset[i];
   }

   /* Reset delay and sound timers */
   chip8->delay_timer = 0;
   chip8->sound_timer = 0;

   return 0;
}

int Load(char * ROM, Chip8 *chip8)
{
   int bufSize = 512;
   int bufSizeRead = 0;
   int i = 0;
   int k = 0;
   char buf[bufSize];

   printf("Opening %s ...\n",ROM);

   if ((chip8->ROMfd = open(ROM,O_RDONLY)) == 0)
   {
      exiterror(2);
   }

   while((bufSizeRead=read(chip8->ROMfd,buf,bufSize))>0)
   {
      for(k=0;k<bufSizeRead;k++)
      {             /* 0x200 */
         chip8->memory[512 + i] = buf[i];
         i = i++;
      }
   }

   /*printf("%x\n",chip8->memory[512]);
   printf("%x\n",chip8->memory[513]);
   printf("%x\n",chip8->memory[514]);
   printf("%x\n",chip8->memory[515]);*/

   if (bufSizeRead == -1)
   {
      exiterror(3);
   }

   close(chip8->ROMfd);

   return 0;
}

int EmulateCycle(Chip8 *chip8)
{
   int opfound = 0;
   int debug = 0;
   int i;

   unsigned short x = chip8->V[(chip8->opcode & 0x0F00) >> 8];
   unsigned short y = chip8->V[(chip8->opcode & 0x00F0) >> 4];
   unsigned short height = chip8->opcode & 0x000F;
   unsigned short pixel;

   /* Fetch */
   chip8->opcode = chip8->memory[chip8->pc] << 8 | chip8->memory[chip8->pc+1];

   if (debug == 1) printf("%x\n",chip8->opcode);

   switch(chip8->opcode & 0xF000)
   {
      case 0xA000:
         chip8->I = chip8->opcode & 0x0FFF;
         if (debug == 1) printf("I = %x\n", chip8->opcode & 0x0FFF);
         chip8->pc = chip8->pc + 2;
         opfound = 1;
      break;

      case 0x6000:
         chip8->V[chip8->opcode & 0x0F00] = chip8->opcode & 0x00FF;
         if (debug == 1) printf("V[%x] = %x\n", (chip8->opcode & 0x0F00) >> 8, chip8->opcode & 0x00FF);
         chip8->pc = chip8->pc + 2;
         opfound = 1;
      break;

      case 0xD000:
         chip8->V[0xF] = 0;
         for (int yline = 0; yline < height; yline++)
         {
            pixel = chip8->memory[chip8->I + yline];
            for(int xline = 0; xline < 8; xline++)
            {
               if((pixel & (0x80 >> xline)) != 0)
               {
                  if(chip8->gfx[(x + xline + ((y + yline) * 64))] == 1)
                  chip8->V[0xF] = 1;                                 
                  chip8->gfx[x + xline + ((y + yline) * 64)] ^= 1;
               }
            }
         }
 
         chip8->DrawFlag = 1;

         if (debug == 1) printf("Draw call %x\n",chip8->opcode);
         chip8->pc = chip8->pc + 2;
         opfound = 1;
      break;

      case 0x2000:
         chip8->stack[chip8->sp] = chip8->pc;
         chip8->sp=chip8->sp++;
         chip8->pc = chip8->opcode & 0x0FFF;
         if (debug == 1) printf("pc = %x\n", chip8->opcode & 0x0FFF);
         opfound = 1;
      break;

      case 0x7000:
         chip8->V[(chip8->opcode & 0x0F00) >> 8] = chip8->V[(chip8->opcode & 0x0F00) >> 8] + (chip8->opcode & 0x00FF);
         if (debug == 1) printf("VX[%x] = %x\n", chip8->opcode & 0x00FF,chip8->V[(chip8->opcode & 0x0F00) >> 8] + (chip8->opcode & 0x00FF));
         chip8->pc = chip8->pc + 2;
         opfound = 1;
      break;
   }

   switch(chip8->opcode & 0xF0FF)
   {
      case 0xF033:
         chip8->memory[chip8->I] = chip8->V[(chip8->opcode & 0x0F00) >> 8] / 100;
         chip8->memory[chip8->I + 1] = (chip8->V[(chip8->opcode & 0x0F00) >> 8] / 10) % 10;
         chip8->memory[chip8->I + 2] = (chip8->V[(chip8->opcode & 0x0F00) >> 8] / 1) % 10;
         if (debug == 1) printf("Mem[%x] = %x\n",chip8->I, chip8->V[(chip8->opcode & 0x0F00) >> 8] / 100);
         if (debug == 1) printf("Mem[%x] = %x\n",chip8->I + 1, (chip8->V[(chip8->opcode & 0x0F00) >> 8] / 10) % 10);
         if (debug == 1) printf("Mem[%x] = %x\n",chip8->I + 2, (chip8->V[(chip8->opcode & 0x0F00) >> 8] / 10) % 1);
         chip8->pc = chip8->pc + 2;
         opfound = 1;
      break;

      case 0xF065:
         for(i=0;i<((chip8->opcode & 0x0F00) >> 8);i++)
         {
            chip8->V[i] = chip8->memory[chip8->I + i];
            if (debug == 1) printf("V[%x] is %x\n",chip8->V[i],chip8->memory[chip8->I + i]);
            chip8->pc = chip8->pc + 2;
            opfound = 1;
         }
      break;

      case 0xF029:
         chip8->I = chip8->memory[chip8->V[(chip8->opcode & 0x0F00) >> 8]];
         if (debug == 1) printf("I is %x\n",chip8->memory[chip8->V[(chip8->opcode & 0x0F00) >> 8]]);
         chip8->pc = chip8->pc + 2;
         opfound = 1;
      break;

/* FX29	Sets I to the location of the sprite for the character in VX. Characters 0-F (in hexadecimal) are represented by a 4x5 font. */

   }

   switch(chip8->opcode & 0x00FF)
   {
      case 0x00EE:
      {
         chip8->sp=chip8->sp - 1;
         chip8->pc = chip8->stack[chip8->sp];
         if (debug == 1) printf("pc = %x\n", chip8->opcode & 0x0FFF);
         opfound = 1;
      }
   }

   if (opfound == 0)
   {
      printf("%x not found.\n",chip8->opcode);
      exiterror(20);
   }

/*0NNN	Calls RCA 1802 program at address NNN.
00E0	Clears the screen.
00EE	Returns from a subroutine.
1NNN	Jumps to address NNN.
2NNN	Calls subroutine at NNN.
3XNN	Skips the next instruction if VX equals NN.
4XNN	Skips the next instruction if VX doesn't equal NN.
5XY0	Skips the next instruction if VX equals VY.
6XNN	Sets VX to NN.
7XNN	Adds NN to VX.
8XY0	Sets VX to the value of VY.
8XY1	Sets VX to VX or VY.
8XY2	Sets VX to VX and VY.
8XY3	Sets VX to VX xor VY.
8XY4	Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't.
8XY5	VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
8XY6	Shifts VX right by one. VF is set to the value of the least significant bit of VX before the shift.[2]
8XY7	Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
8XYE	Shifts VX left by one. VF is set to the value of the most significant bit of VX before the shift.[2]
9XY0	Skips the next instruction if VX doesn't equal VY.
ANNN	Sets I to the address NNN.
BNNN	Jumps to the address NNN plus V0.
CXNN	Sets VX to a random number and NN.
DXYN	Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels. Each row of 8 pixels is read as bit-coded (with the most significant bit of each byte displayed on the left) starting from memory location I; I value doesn't change after the execution of this instruction. As described above, VF is set to 1 if any screen pixels are flipped from set to unset when the sprite is drawn, and to 0 if that doesn't happen.
EX9E	Skips the next instruction if the key stored in VX is pressed.
EXA1	Skips the next instruction if the key stored in VX isn't pressed.
FX07	Sets VX to the value of the delay timer.
FX0A	A key press is awaited, and then stored in VX.
FX15	Sets the delay timer to VX.
FX18	Sets the sound timer to VX.
FX1E	Adds VX to I.[3]
FX29	Sets I to the location of the sprite for the character in VX. Characters 0-F (in hexadecimal) are represented by a 4x5 font.
FX33	Stores the Binary-coded decimal representation of VX, with the most significant of three digits at the address in I, the middle digit at I plus 1, and the least significant digit at I plus 2. (In other words, take the decimal representation of VX, place the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.)
FX55	Stores V0 to VX in memory starting at address I.[4]
FX65	Fills V0 to VX with values from memory starting at address I.[4] */

   /* Decode */

   /* Execute */
   return 0;
}

int main(int argc, char **argv)
{
   Chip8 chip8;
   Display display;
   int KeyValue = 0;

   /* 0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
      0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
      0x200-0xFFF - Program ROM and work RAM
   */

   InitScreen(&display);
   InitCPU(&chip8);
   Load("/home/user/c/chip-8/roms/pong.ch8", &chip8);

   /* Emulation loop */
   while(KeyValue != 'q')
   {
      /* Fetch, decode, execute */
      EmulateCycle(&chip8);

      if (chip8.DrawFlag)
      {
         chip8.DrawFlag = 0;
         UpdateGraphics(&chip8,&display);
      }
   }

   return 0;
}