#include <SFML/Graphics.hpp>
#include <math.h>
#include<iostream>
using namespace sf;

int width = 1024;
int height = 768;
int roadW = 2000;
int segL = 200; //segment length
float camD = 0.84; // camera depth
int speed = 200;

struct Line
{
	float x, y, z; // 3d center of line
	float X, Y, W; // screen cord
	float scale, curve, spriteX, clip;
	Sprite sprite;

	// constructor 
	Line(){spriteX=curve=x=y=z=0;}

	// from world to screen coordinates
	void project(int camX, int camY, int camZ)
	{
		scale = camD/(z-camZ);
		X = (1+scale*(x-camX))*width/2;
		Y = (1-scale*(y-camY))*height/2;
		W = scale * roadW * width/2;
	}

	void drawSprite(RenderWindow &app)
	{
		Sprite s = sprite;
		int w = s.getTextureRect().width;
		int h = s.getTextureRect().height;

		float destX = X + scale * spriteX * width/2;
		float destY = Y + 4;
		float destW = w * W /266;
		float destH = h * W /266;

		destX += destW * spriteX; // offsetX
		destY += destH * (-1); // offsetY

		float clipH = destY + destH - clip;
		if(clipH<0) clipH = 0;

		if(clipH>=destH) return;
		s.setTextureRect(IntRect(0,0,w,h-h*clipH/destH));
		s.setScale(destW/w,destH/h);
		s.setPosition(destX,destY);
		app.draw(s);
	
	}
};

// &w means reference of w, renference is like a nickname of a variable
// x1 is x of center of bottom line, x2 is x of center of upper line
// w1 is bottom width,  w2 is upper width
// y1 is y of center of bottom line, y2 is y of center of upper line

void drawQuad(RenderWindow &w, Color c, int x1, int y1, int w1, int x2, int y2, int w2)
{
	// parameter: point count
	ConvexShape shape(4);
	
	shape.setFillColor(c);

	// upper left point
	shape.setPoint(0,Vector2f(x1-w1,y1));

	// bottom left point
	shape.setPoint(1,Vector2f(x2-w2,y2));

	// bottom right point
	shape.setPoint(2,Vector2f(x2+w2,y2));

	// upper right point
	shape.setPoint(3,Vector2f(x1+w1,y1));
	w.draw(shape);
} 

int main()
{
	
	RenderWindow app(VideoMode(width,height),"3D Car Racing!");
	app.setFramerateLimit(60);

	// background
	Texture bg;
	bg.loadFromFile("images/bg.png");
	bg.setRepeated(true);
	Sprite sBackground(bg);

	// set part of texture to be displayed
	sBackground.setTextureRect(IntRect(0,0,5000,411));

	// set position of background
    sBackground.setPosition(-2000,0);

	Sprite sprites[8];
	Texture textures[8];

	for(int i=1;i<8;i++)
	{
		textures[i].loadFromFile("images/"+std::to_string(i)+".png");
		textures[i].setSmooth(true);
		sprites[i].setTexture(textures[i]);
	}
	
	std::vector<Line> lines;

	for(int i = 0;i<2000;i++)
	{
		Line line;
		line.z = i*segL;

		// curve road

		if (i>100 && i<400) line.curve = 0.5;
		if (i>500 && i<800) line.curve = -0.5;

		// for slope, i/n must start and end at pi rad, which causes line.y to form 0 (sin (pi rad)=0)
		if (i>=754 && i<1131) 
		{
			// sin (i/30) = y/1500
			// sin method is in radians
			line.y = sin(i/30.0)*1500;
			//std::cout<<line.y<<"  "<<i<<"\n";
		}

		// bigger slope
		if(i>=1131&&i<=1508) line.y = sin(i/60.0)*6000;

		// initializing objects

		// grass on the right
		if(i%10==0){line.spriteX=1.5; line.sprite=sprites[6];}
		// trees on the left
		if(i%20==0&&i<200){line.spriteX=-2.5; line.sprite=sprites[1];}
		if(i%20==0&&i>=200){line.spriteX=-2.5; line.sprite=sprites[2];}
		if(i%20==0&&i>=500){line.spriteX=-2.5; line.sprite=sprites[1];}
		// houses on the right
		if(i%500==0){line.spriteX=-2.5; line.sprite=sprites[7];}

		// adds data to end of vector
		lines.push_back(line);
	}
	
	int N = lines.size();

	// current position
	int pos = 0;

	// player x position
	int playerX = 0;

	while(app.isOpen())
	{
		Event e;
		while(app.pollEvent(e))
		{
			if(e.type==Event::Closed) app.close();
		}
	
	
		app.clear();
		app.draw(sBackground);

		// keyboard events

		if(Keyboard::isKeyPressed(Keyboard::Up)) pos+=speed;

		
		if(Keyboard::isKeyPressed(Keyboard::Left)) playerX-=200;
		
		if(Keyboard::isKeyPressed(Keyboard::Right)) playerX+=200;
		
		//if(pos>0)
		//{
			if(Keyboard::isKeyPressed(Keyboard::Down)) pos-=speed;
		//}

		// prevent out of bounds
		while(pos>=N*segL) pos-=N*segL;
		while(pos<0) pos+=N*segL;
		
		// index of first segment on the screen to be rendered
		int startPos = pos / segL;

		// camera height
		int camH = 1500 + lines[startPos].y;

		int maxY = height;

		// x is direction of car, dx is magnitude of curve
		float x = 0, dx =0;


		// draw road

		for(int n=startPos;n<startPos+300;n++)
		{
			Line &l = lines[n%N];

			// road is pulled to the left
			// if it is out of the total length of map, the map is repeated
			l.project(playerX-x,camH,pos-(n>=N?N*segL:0));

			x+=dx;
			dx+=l.curve;

			l.clip = maxY;

			if (l.Y>=maxY) continue;

			// setting camera where the front slope should block the slope behind
			maxY = l.Y;

			Color grass = (n/3)%2?Color(16,200,16):Color(0,154,0);
			Color rumble = (n/3)%2?Color(255,255,255):Color(0,0,0);
			Color road = (n/3)%2?Color(107,107,107):Color(105,105,105);
		
			Line p = lines[(n-1)%N]; // previous line

			drawQuad(app,grass,0,p.Y,width,0,l.Y,width);
			drawQuad(app,rumble,p.X,p.Y,p.W*1.2,l.X,l.Y,l.W*1.2);
			drawQuad(app,road,p.X,p.Y,p.W,l.X,l.Y,l.W);

		}
		//drawQuad(app,Color::Green,500,500,200,500,300,100);

		// draw objects
		for(int n=startPos+300;n>startPos;n--)
		{
			lines[n%N].drawSprite(app);
		}
		
		app.display();
		
	}
	
	return 0;
}