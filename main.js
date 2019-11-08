
const Imports = {};

//const MemoryPageCount = 2048;
//const MemorySize = MemoryPageCount * 65536;
//Imports.memory = new WebAssembly.Memory({initial:MemoryPageCount});
let WasmMemory = null;//new Uint8Array(Imports.memory.buffer);
const GLObjects = new Array();
const ASCIIDecoder = new TextDecoder("ascii");



async function LoadImageAsRawPixels(FileName)
{
    const Img = new Image();
    Img.src = FileName;
    await Img.decode();

    const DummyCanvas = document.createElement("canvas");
    const Context = DummyCanvas.getContext("2d");

    DummyCanvas.width = Img.width;
    DummyCanvas.height = Img.height;
    Context.drawImage(Img, 0, 0);

    const Data = Context.getImageData(0, 0, Img.width, Img.height).data.buffer;

    DummyCanvas.remove();
    return Data;
}

let FontAtlasPixels = LoadImageAsRawPixels("font_atlas_signed_distance_field.png");
let FontAtlasGeom = fetch("font_atlas_signed_distance_field.geom").then(data => data.arrayBuffer());
let Program = fetch("main.wasm");

const gl = document.getElementById("canvas_webgl").getContext("webgl2");
if(!gl)
{
    let Message = "WebGL 2 context could not be acquired";
    document.body.innerHTML = Message;
    throw Error(Message);
}



function GetWasmString(Length, Pointer)
{
    const Data = WasmMemory.subarray(Pointer, Pointer+Length);
    return ASCIIDecoder.decode(Data);
}

function PushGLObj(Obj)
{
    GLObjects.push(Obj);
    return GLObjects.length - 1;
}


Imports.JS_Log = function(MsgLength, MsgPtr)
{
    console.log("main.wasm: " + GetWasmString(MsgLength, MsgPtr));
}

Imports.JS_Abort = function(ReasonLength, ReasonPtr,
                            FileNameLength, FileNamePtr,
                            FuncNameLength, FuncNamePtr,
                            LineNumber)
{
    const Reason = GetWasmString(ReasonLength, ReasonPtr);
    const FileName = GetWasmString(FileNameLength, FileNamePtr);
    const FuncName = GetWasmString(FuncNameLength, FuncNamePtr);
    const Message = "ABORT: in function: " + FuncName + " (" + FileName + ":" + LineNumber + "): " + Reason;
    document.body.innerHTML = "<pre>" + Message + "</pre>";
    throw new Error(Message);
}


Imports.glClear = function(Mask) { gl.clear(Mask); }
Imports.glClearColor = function(R, G, B, A) { gl.clearColor(R, G, B, A); }
Imports.glViewport = function(x, y, w, h) { gl.viewport(x,y,w,h,); }
Imports.glBlendFunc = function(SourceFactor, DestFactor) { gl.blendFunc(SourceFactor, DestFactor); }
Imports.glBlendEquation = function(Mode) { gl.blendEquation(Mode); }
Imports.glCreateShader = function(Type) { return PushGLObj(gl.createShader(Type)); }
Imports.glCreateProgram = function() { return PushGLObj(gl.createProgram()); }
Imports.glAttachShader = function(Program, Shader) { gl.attachShader(GLObjects[Program], GLObjects[Shader]); }
Imports.glLinkProgram = function(Program) { gl.linkProgram(Prog); }
Imports.glUseProgram = function(Program) { gl.useProgram(GLObjects[Program]); }
Imports.glCreateVertexArray = function(Program) {return PushGLObj(gl.createVertexArray()) };
Imports.glCreateBuffer = function(Program) {return PushGLObj(gl.createBuffer()) };
Imports.glBindBuffer = function(Type, Buffer) { gl.bindBuffer(Type, GLObjects[Buffer]); }
Imports.glBindVertexArray = function(Arr) { gl.bindVertexArray(GLObjects[Arr]); }
Imports.glEnableVertexAttribArray = function(Index) { gl.enableVertexAttribArray(Index) };
Imports.glDrawArrays = function(Mode, First, Count) { gl.drawArrays(Mode, First, Count); }
Imports.glDrawElements = function(Mode, Count, Type, Offset) { gl.drawElements(Mode, Count, Type, Offset); }
Imports.glEnable = function(Mode) { gl.enable(Mode); }
Imports.glDisable = function(Mode) { gl.disable(Mode); }
Imports.glCreateTexture = function() { return PushGLObj(gl.createTexture()); }
Imports.glBindTexture = function(Target, Texture) { gl.bindTexture(Target, GLObjects[Texture]); }
Imports.glGenerateMipmap = function(Target) { gl.generateMipmap(Target); }
Imports.glTexParameteri = function(Target, Var, Value) { gl.texParameteri(Target, Var, Value); }
Imports.glActiveTexture = function(Texture) { gl.activeTexture(Texture); }
Imports.glUniform1i = function(Location, Value) { gl.uniform1i(GLObjects[Location], Value); }
Imports.glUniform3f = function(Location, x,y,z) { gl.uniform3f(GLObjects[Location], x,y,z); }

Imports.glGetUniformLocation = function(Program, NameLen, NamePtr)
{
    return PushGLObj(gl.getUniformLocation(GLObjects[Program], GetWasmString(NameLen, NamePtr)));
}

Imports.glUniformMatrix4fv = function(Loc, Transpose, DataPtr)
{
    const Data = new Float32Array(WasmMemory.buffer, DataPtr, 16);
    gl.uniformMatrix4fv(GLObjects[Loc], Transpose, Data);
}

Imports.glGetAttribLocation = function(Program, NameLength, NamePtr)
{
    return gl.getAttribLocation(GLObjects[Program], GetWasmString(NameLength, NamePtr)); 
}

Imports.glVertexAttribPointer = function(Index, Size, Type, Normalized, Stride, Offset) 
{
    gl.vertexAttribPointer(Index, Size, Type, Normalized, Stride, Offset);
}

Imports.glShaderSource = function(ShaderID, SourceLen, SourcePtr)
{
    return gl.shaderSource(GLObjects[ShaderID], GetWasmString(SourceLen, SourcePtr));
}

Imports.glCompileShader = function(ShaderIndex)
{
    const Shader = GLObjects[ShaderIndex];
    gl.compileShader(Shader);
    if(!gl.getShaderParameter(Shader, gl.COMPILE_STATUS))
    {
        throw new Error(gl.getShaderInfoLog(Shader));
    }
}


Imports.glBufferData = function(Target, Size, DataPtr, UsageHint)
{
    if(DataPtr != 0)
    {
        const Data = WasmMemory.subarray(DataPtr, DataPtr+Size);
        gl.bufferData(Target, Data, UsageHint); // Allocate&upload to GPU
    }
    else
    {
        gl.bufferData(Target, Size, UsageHint); // Only allocate
    }
}

Imports.glBufferSubData = function(Target, Offset, Size, DataPtr)
{
    const Data = WasmMemory.subarray(DataPtr, DataPtr+Size);
    gl.bufferSubData(Target, Offset, Data);
}

Imports.glTexImage2D = function(Target, Level, InternalFormat, Width, Height, Border, Format, Type, DataPtr, DataSize, Offset)
{
    const Data = WasmMemory.subarray(DataPtr, DataPtr + DataSize);
    gl.texImage2D(Target, Level, InternalFormat, Width, Height, Border, Format, Type, Data, Offset);
}




function CompileShader(Type, SourceLen, SourcePtr)
{
    const Shader = gl.createShader(Type);
    const Source = GetWasmString(SourceLen, SourcePtr);
    gl.shaderSource(Shader, Source);
    gl.compileShader(Shader);
    if(!gl.getShaderParameter(Shader, gl.COMPILE_STATUS))
    {
        const Message = gl.getShaderInfoLog(Shader);
        document.body.innerHTML = "<b><pre>" + Message + "</pre></b><pre>" + "\n\n" + Source + "</pre>";
        throw new Error(Message);
    }
    return Shader;
}

Imports.JS_GL_CreateCompileAndLinkProgram = function(VertLen, VertPtr, FragLen, FragPtr)
{
    const Vert = CompileShader(gl.VERTEX_SHADER, VertLen, VertPtr);
    const Frag = CompileShader(gl.FRAGMENT_SHADER, FragLen, FragPtr);
    const Program = gl.createProgram();
    gl.attachShader(Program, Vert);
    gl.attachShader(Program, Frag);
    gl.linkProgram(Program);
    gl.deleteShader(Vert);
    gl.deleteShader(Frag);
    if(!gl.getProgramParameter(Program, gl.LINK_STATUS))
    {
        const Message = "WebGL link error: " + gl.getProgramInfoLog(Program);
        document.body.innerHTML = Message;
        throw new Error(Message);
    }

    return PushGLObj(Program);
}

Imports.JS_GetFontAtlas = function(PixelsPtr, PixelsSize, GeomPtr, GeomSize)
{
    if(PixelsSize != FontAtlasPixels.byteLength)
        throw Error("Invalid font atlas pixel buffer size, got :"+PixelsSize+",  expected "+FontAtlasPixels.byteLength);
    if(GeomSize != FontAtlasGeom.byteLength)
        throw Error("Invalid font atlas geom buffer size, got :"+GeomSize+",  expected "+FontAtlasGeom.byteLength);

    new Uint8Array(WasmMemory.buffer, PixelsPtr, PixelsSize).set(new Uint8Array(FontAtlasPixels, 0, PixelsSize));
    new Uint8Array(WasmMemory.buffer, GeomPtr, GeomSize).set(new Uint8Array(FontAtlasGeom, 0, GeomSize));
}


async function Init()
{
    Program = WebAssembly.instantiateStreaming(Program, {"env":Imports});

    FontAtlasGeom = await FontAtlasGeom;
    FontAtlasPixels = await FontAtlasPixels;
    Program = await Program;

    const WasmExports = Program.instance.exports;
    WasmMemory = new Uint8Array(WasmExports.memory.buffer);

    function ResizeHandler()
    {
        const Ratio = window.devicePixelRatio;
        const Width = window.innerWidth;
        const Height = window.innerHeight;
        gl.canvas.width = Width;
        gl.canvas.height = Height;
        console.log("Resize");
    }
    window.addEventListener('resize', ResizeHandler);

    gl.canvas.addEventListener("mousemove", Event =>
    {
        const Rect = gl.canvas.getBoundingClientRect();
        const MouseX = Event.clientX - Rect.left;
        const MouseY = Event.clientY - Rect.top;
        WasmExports.MouseMove(MouseX, MouseY);
    });

    gl.canvas.addEventListener("mousedown", Event =>
    {
        WasmExports.MouseLeft(true);
    });

    gl.canvas.addEventListener("mouseup", Event =>
    {
        WasmExports.MouseLeft(false);
    });

    window.addEventListener("keydown", Event =>
    {
        WasmExports.KeyPress(Event.keyCode, true);
    });

    window.addEventListener("keyup", Event =>
    {
        WasmExports.KeyPress(Event.keyCode, false);
    });


    WasmExports.Init();
    const AllExtensions = gl.getSupportedExtensions();
    console.log(AllExtensions);

    let PrevTime = null;
    function RenderLoop(Now)
    {
        window.requestAnimationFrame(RenderLoop);
        const DeltaTime = PrevTime ? Now - PrevTime : Now;
        PrevTime = Now;
        WasmExports.UpdateAndRender(gl.canvas.width, gl.canvas.height, DeltaTime*0.001);
    }

    ResizeHandler();
    window.requestAnimationFrame(RenderLoop);
}

Init();

