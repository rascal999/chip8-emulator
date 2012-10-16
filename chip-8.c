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
   unsigned char gfx[64][32]; /* Graphics */
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
      case 30:
         printf("Error 30: Could not initialise screen\n");
         exit(30);
         break;
      case 40:
         printf("Error 40: Could not draw to screen\n");
         exit(40);
         break;
      default:
         printf("Error: Unknown error code\n");
         exit(1);
         break;
   }
}

/* Screen functions */
void setpixel(Display * display, int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
    Uint32 *pixmem32;
    Uint32 colour;  
 
    colour = SDL_MapRGB( display->screen->format, r, g, b );
  
    pixmem32 = (Uint32*) display->screen->pixels  + y + x;
    *pixmem32 = colour;
}

/*void DrawScreen(SDL_Surface* screen, int x, int y, int c)*/
int DrawScreen(Display * display, int x, int y, int c)
{
   int ytimesw;
   int blocky;
   int blockx;
   x = x * BLOCK;
   y = y * BLOCK;

   if (c == 1)
   {
      c = 128;
   } else {
      c = 0;
   }
  
   if (SDL_MUSTLOCK(display->screen)) 
   {
      if(SDL_LockSurface(display->screen) < 0) return 1;
   }

   for(blocky=0;blocky<BLOCK;blocky++)
   {
      ytimesw = y*display->screen->pitch/BPP;
      for(blockx=0;blockx<BLOCK;blockx++)
      {
         setpixel(display, blockx + x, (blocky*(display->screen->pitch/BPP)) + ytimesw, c,c,c);
      }
   }

   if(SDL_MUSTLOCK(display->screen)) SDL_UnlockSurface(display->screen);
  
   SDL_Flip(display->screen); 

   return 0;
}

int UpdateGraphics(Chip8 * chip8, Display * display)
{
   int i = 0;
   int x, y;

   for (y = 0; y < 32; y++)
   {
      for (x = 0; x < 64; x++)
      {
            if(chip8->gfx[x][y] != 0)
            {
               DrawScreen(display,x,y,1);
//printf("x %d y %d\n",x,y);
            }
       }
    }

   return 0;
}

int InitScreen(Display * display)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0 ) return 1;

    if (!(display->screen = SDL_SetVideoMode(WIDTH, HEIGHT, DEPTH, SDL_HWSURFACE)))
    {
        SDL_Quit();
        return 1;
    }

    return 0;
}

/* END Screen functions */

int DebugOutput(Chip8 *chip8)
{
   int i;

   printf("------------------\n");

   printf("pc = %x\n",chip8->pc);
   printf("I = %x\n",chip8->I);

   for(i=0;i<=0xF;i++)
   {
      printf("V[%x] = %x\n",i,chip8->V[i]);
   }

   printf("------------------\n");

   return 0;
}

int InitCPU(Chip8 *chip8)
{
   int i;
   int x, y;

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
   for (y=0;y<32;y++)
   {
      for (x=0;x<64;x++)
      {
         chip8->gfx[x][y] = 0;
      }
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
         i++;
      }
   }

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
   int i, x;

   unsigned short xcoord = 0;
   unsigned short ycoord = 0;
   unsigned short height = 0;
   unsigned short pixel;

   /* Fetch */
   chip8->opcode = chip8->memory[chip8->pc] << 8 | chip8->memory[chip8->pc+1];

   if (debug == 1) printf("%x\n",chip8->opcode);

   switch(chip8->opcode & 0xF000)
   {
           case 0x1000:
                   chip8->pc=chip8->opcode & 0x0FFF;
                   if (debug == 1) printf("pc = %x\n",chip8->opcode & 0x0FFF);
                   opfound = 1;
           break;
	   case 0xA000: /* Checked */
		   chip8->I = chip8->opcode & 0x0FFF;
		   if (debug == 1) printf("I = %x\n", chip8->opcode & 0x0FFF);
		   chip8->pc = chip8->pc + 2;
		   opfound = 1;
	   break;


           case 0x4000:
                   if (chip8->V[(chip8->opcode & 0x0F00) >> 8] != chip8->opcode & 0x00FF)
                   {
		      chip8->pc = chip8->pc + 4;
                   } else {
		      chip8->pc = chip8->pc + 2;
                   }

                   if (debug == 1) printf("pc = %x\n",chip8->pc);

		   opfound = 1;
	   break;
                   
/* 4XNN    Skips the next instruction if VX doesn't equal NN. */

           case 0xC000:
                   /* 5 should be a random number */
                   chip8->V[(chip8->opcode & 0x0F00) >> 8] = 5 & (chip8->opcode & 0x00FF);
                   if (debug == 1) printf("V[%x] = %x",(chip8->opcode & 0x0F00) >> 8,5 & (chip8->opcode & 0x00FF));
		   chip8->pc = chip8->pc + 2;
		   opfound = 1;
           break;

/* Cxkk - RND Vx, byte
Set Vx = random byte AND kk.

The interpreter generates a random number from 0 to 255, which is then ANDed with the value kk. The results are stored in Vx. See instruction 8xy2 for more information on AND. */

	   case 0x6000: /* Checked */
		   chip8->V[(chip8->opcode & 0x0F00) >> 8] = chip8->opcode & 0x00FF;
		   if (debug == 1) printf("V[%x] = %x\n", (chip8->opcode & 0x0F00) >> 8, chip8->opcode & 0x00FF);
		   chip8->pc = chip8->pc + 2;
		   opfound = 1;
		   break;

	   case 0xD000:
		   chip8->V[0xF] = 0;
                   height = chip8->opcode & 0x000F;
                   xcoord = chip8->V[(chip8->opcode & 0x0F00) >> 8];
                   ycoord = chip8->V[(chip8->opcode & 0x00F0) >> 4];

		   for (i=0;i<height;i++)
		   {
                      pixel = chip8->memory[chip8->I + i];
                      //printf("sprite %x\n",pixel);
                      for (x=0;x<8;x++)
                      {
                         if ((pixel & (0x80 >> x)) != 0)
                         {
//printf("x %d y %d px %x\n",xcoord,ycoord,pixel);
                            //printf("%x %d %d\n",chip8->opcode,xcoord,ycoord);
                            if (chip8->gfx[xcoord+x][ycoord+i] == 1) chip8->V[0xF] = 1;
                            chip8->gfx[xcoord+x][ycoord+i] ^= 1;
                         }
                      }
                   }

         chip8->DrawFlag = 1;

         if (debug == 1) printf("Draw call %x\n",chip8->opcode);
         chip8->pc = chip8->pc + 2;
         opfound = 1;
      break;

      case 0x2000: /* Checked */
         chip8->stack[chip8->sp] = chip8->pc;
         chip8->sp++;
         chip8->pc = chip8->opcode & 0x0FFF;
         if (debug == 1) printf("pc = %x\n", chip8->opcode & 0x0FFF);
         opfound = 1;
      break;

      case 0x3000:
         if (chip8->V[(chip8->opcode & 0x0F00) >> 8] == (chip8->opcode & 0x00FF))
         {
            chip8->pc = chip8->pc + 4;
         } else {
            chip8->pc = chip8->pc + 4;
         }
         opfound = 1;
         if (debug == 1) printf("V[%x] = %x\n",(chip8->opcode & 0x0F00) >> 8,chip8->V[(chip8->opcode & 0x0F00) >> 8]);
      break;

/* 3XNN	Skips the next instruction if VX equals NN. */

      case 0x7000: /* Checked */
         chip8->V[(chip8->opcode & 0x0F00) >> 8] = chip8->V[(chip8->opcode & 0x0F00) >> 8] + (chip8->opcode & 0x00FF);
         if (debug == 1) printf("V[%x] = %x\n", (chip8->opcode & 0x0F00) >> 8,chip8->V[(chip8->opcode & 0x00FF) >> 8] + (chip8->opcode & 0x00FF));
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
         if (debug == 1) printf("I,I+1,I+2 = %d%d%d\n",chip8->memory[chip8->I],chip8->memory[chip8->I+1],chip8->memory[chip8->I+2]);
         chip8->pc = chip8->pc + 2;
         opfound = 1;
      break;

      case 0xE0A1:
         if (chip8->key[chip8->V[(chip8->opcode & 0x0F00) >> 8]] != 1)
         {
            chip8->pc = chip8->pc + 4;
         } else {
            chip8->pc = chip8->pc + 2;
         }

         if (debug == 1) printf("key[%x] = %x\n",chip8->V[(chip8->opcode & 0x0F00) >> 8],chip8->key[chip8->V[(chip8->opcode & 0x0F00) >> 8]]);
         opfound = 1;
      break;

/* ExA1 - SKNP Vx
Skip next instruction if key with the value of Vx is not pressed.

Checks the keyboard, and if the key corresponding to the value of Vx is currently in the up position, PC is increased by 2. */

      case 0xF007:
         chip8->V[(chip8->opcode & 0x0F00) >> 8] = chip8->delay_timer;
         if (debug == 1) printf("V[%x] = %x\n",(chip8->opcode & 0x0F00) >> 8,chip8->delay_timer);
         chip8->pc = chip8->pc + 2;
         opfound = 1;
      break;

/* FX07	Sets VX to the value of the delay timer. */

      case 0xF015:
         chip8->delay_timer = chip8->V[(chip8->opcode & 0x0F00) >> 8];
         if (debug == 1) printf("delay_timer = %x\n",chip8->V[(chip8->opcode & 0x0F00) >> 8]);
         chip8->pc = chip8->pc + 2;
         opfound = 1;
      break;

/* FX15	Sets the delay timer to VX. */

      case 0xF065:
         for(i=0;i<=((chip8->opcode & 0x0F00) >> 8);i++)
         {
            chip8->V[i] = chip8->memory[chip8->I + i];
            if (debug == 1) printf("V[%x] is %x\n",i,chip8->memory[chip8->I + i]);
            opfound = 1;
         }
         chip8->pc = chip8->pc + 2;
      break;

      case 0xF029:
         //chip8->I = chip8->memory[chip8->V[(chip8->opcode & 0x0F00) >> 8]*5];
         chip8->I = chip8->V[(chip8->opcode & 0x0F00) >> 8]*5;
         if (debug == 1) printf("I is %x\n",chip8->I);
         chip8->pc = chip8->pc + 2;
         opfound = 1;
      break;

/* FX29	Sets I to the location of the sprite for the character in VX. Characters 0-F (in hexadecimal) are represented by a 4x5 font. */

      case 0xF055:
         for(i=0;i<(chip8->opcode & 0x0F00) >> 8;i++)
         {
            unsigned short * ir = chip8->I+i;
            ir=chip8->V[i];
            if (debug == 1) printf("V[%x] = %x\n",i,chip8->I+i);
         }
         chip8->pc = chip8->pc + 2;
         opfound = 1;
      break;
            
/* FX55	Stores V0 to VX in memory starting at address I.[4] */

   }

   switch(chip8->opcode & 0x00FF)
   {
      case 0x00EE:
         chip8->sp=chip8->sp - 1;
         chip8->pc = chip8->stack[chip8->sp];
         if (debug == 1) printf("pc = %x\n", chip8->opcode & 0x0FFF);
         chip8->pc = chip8->pc + 2;
         opfound = 1;
      break;
   }

   switch(chip8->opcode & 0xF00F)
   {
      case 0x8000: /* 0x8XY0 */
         chip8->V[(chip8->opcode & 0x0F00) >> 8] = chip8->V[(chip8->opcode & 0x00F0) >> 4];
         if (debug == 1) printf("V[%x] = %x\n",(chip8->opcode & 0x0F00) >> 8,chip8->V[(chip8->opcode & 0x0F00) >> 8]);
         chip8->pc = chip8->pc + 2;
         opfound = 1;
      break;

/* 8XY0	Sets VX to the value of VY. */

      case 0x8002:
         chip8->V[(chip8->opcode & 0x0F00) >> 8] = chip8->V[(chip8->opcode & 0x0F00) >> 8] & chip8->V[(chip8->opcode & 0x00F0) >> 4];
         if (debug == 1) printf("V[%x] = %x\n",(chip8->opcode & 0x0F00) >> 8,chip8->V[(chip8->opcode & 0x0F00) >> 8]);
         chip8->pc = chip8->pc + 2;
         opfound = 1;
      break;
 
/* 8XY2    Sets VX to VX and VY. */

      case 0x8004:
         chip8->V[(chip8->opcode & 0x0F00) >> 8] = chip8->V[(chip8->opcode & 0x0F00) >> 8] + chip8->V[(chip8->opcode & 0x00F0) >> 4];
         if (chip8->V[(chip8->opcode & 0x0F00) >> 8] + chip8->V[(chip8->opcode & 0x00F0) >> 4] > 255)
         {
            chip8->V[0xF] = 1;
         }

         if (debug == 1) printf("V[%x] = %x. V[F] = %x\n",(chip8->opcode & 0x0F00) >> 8,chip8->V[(chip8->opcode & 0x0F00) >> 8],chip8->V[0xF]);

         chip8->pc = chip8->pc + 2;
         opfound = 1;
      break;

/* 8xy4 - ADD Vx, Vy
Set Vx = Vx + Vy, set VF = carry.

The values of Vx and Vy are added together. If the result is greater than 8 bits (i.e., > 255,) VF is set to 1, otherwise 0. Only the lowest 8 bits of the result are kept, and stored in Vx. */

/* 8XY4    Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't. */

   }

   if(chip8->delay_timer > 0)
   {
      chip8->delay_timer=chip8->delay_timer - 1;
   }
 
   if(chip8->sound_timer > 0)
   {
      if(chip8->sound_timer == 1)
      printf("BEEP!\n");
      chip8->sound_timer=chip8->sound_timer - 1;
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
   /* Chip8 struct */
   Chip8 chip8;

   /* Display struct */
   Display display;

   /* Screen struct for Display */
   SDL_Surface screen;

   SDL_Event event;

   /* Assign screen to screenptr */
   display.screen = &screen;
   display.event = event;

   int KeyValue = 0;

   /* 0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
      0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
      0x200-0xFFF - Program ROM and work RAM
   */

   if (InitScreen(&display) != 0) exiterror(30);
   InitCPU(&chip8);
   Load("/home/user/git/chip8-emulator/roms/pong.ch8", &chip8);
   //Load("/home/user/git/chip8-emulator/roms/ttt.ch8", &chip8);

   /* Emulation loop */
   while(KeyValue != 'q')
   {
      /* Fetch, decode, execute */
      EmulateCycle(&chip8);
      //DebugOutput(&chip8);
      //usleep(500);

      if (chip8.DrawFlag)
      {
         chip8.DrawFlag = 0;
         UpdateGraphics(&chip8,&display);
      }
   }

   return 0;
}
