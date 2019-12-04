
firebase.initializeApp(firebaseConfig);

const Imports = {};

//const MemoryPageCount = 2048;
//const MemorySize = MemoryPageCount * 65536;
//Imports.memory = new WebAssembly.Memory({initial:MemoryPageCount});
//let WasmMemory = null;//new Uint8Array(Imports.memory.buffer);
let WasmMemory = null;
let WasmExports = null;
const GLObjects = new Array();
GLObjects.push(null);
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

let SkyboxFaces = new Array();
SkyboxFaces.push(LoadImageAsRawPixels("skybox/right1.png"));
SkyboxFaces.push(LoadImageAsRawPixels("skybox/left2.png"));
SkyboxFaces.push(LoadImageAsRawPixels("skybox/top3.png"));
SkyboxFaces.push(LoadImageAsRawPixels("skybox/bottom4.png"));
SkyboxFaces.push(LoadImageAsRawPixels("skybox/front5.png"));
SkyboxFaces.push(LoadImageAsRawPixels("skybox/back6.png"));

let BusMesh = fetch("bus.bin").then(data => data.arrayBuffer());
let Program = fetch("main.wasm");
let FontAtlasPixels = LoadImageAsRawPixels("font_atlas_signed_distance_field.png");
let FontAtlasGeom = fetch("font_atlas_signed_distance_field.geom").then(data => data.arrayBuffer());

/*
let TestData = null;
async function GetTestDataFromJson(path)
{
    TestData = await fetch(path).then(data => data.json());
}
GetTestDataFromJson("generated.json");
*/

const gl = document.getElementById("canvas_webgl")
           .getContext("webgl2",
{ 
  alpha: true, 
  antialias: false, 
  depth: true, 
  failIfMajorPerformanceCaveat: false, 
  powerPreference: "default",
  premultipliedAlpha: false, 
  preserveDrawingBuffer: false, 
  stencil: false,
  desynchronized: false
});

if(!gl)
{
    let Message = "WebGL 2 context could not be acquired";
    document.body.innerHTML = Message;
    throw Error(Message);
}


console.log(gl.getContextAttributes());

gl.getExtension("EXT_color_buffer_float");
gl.getExtension("EXT_float_blend");


function GetWasmString(Length, Pointer)
{
    const Data = new Uint8Array(WasmMemory.buffer, Pointer, Length);
    const Str = ASCIIDecoder.decode(Data);
    //console.log(Pointer);
    //console.log("Str: " + Str);
    return Str;
}

function AddJSStringToWasm(Str)
{
    const Ptr = WasmExports.AllocatePermanent(Str.length);
    const Dest = new Uint8Array(WasmMemory.buffer, Ptr, Str.length);
    for(let i = 0; i < Str.length; ++i)
    {
        Dest[i] = Str.charCodeAt(i);
    }
    return Ptr;
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
Imports.glEnableVertexAttribArray = function(Index) { gl.enableVertexAttribArray(Index); };
Imports.glVertexAttribDivisor = function(Index, Divisor) { gl.vertexAttribDivisor(Index, Divisor); };
Imports.glDrawArrays = function(Mode, First, Count) { gl.drawArrays(Mode, First, Count); }
Imports.glDrawArraysInstanced = function(Mode, First, Count, InstanceCount) { gl.drawArraysInstanced(Mode, First, Count, InstanceCount); }
Imports.glDrawElements = function(Mode, Count, Type, Offset) { gl.drawElements(Mode, Count, Type, Offset); }
Imports.glDrawElementsInstanced = function(Mode, Count, Type, Offset, InstanceCount) { gl.drawElementsInstanced(Mode, Count, Type, Offset, InstanceCount); }
Imports.glEnable = function(Mode) { gl.enable(Mode); }
Imports.glDisable = function(Mode) { gl.disable(Mode); }
Imports.glCreateTexture = function() { return PushGLObj(gl.createTexture()); }
Imports.glBindTexture = function(Target, Texture) { gl.bindTexture(Target, GLObjects[Texture]); }
Imports.glGenerateMipmap = function(Target) { gl.generateMipmap(Target); }
Imports.glTexParameteri = function(Target, Var, Value) { gl.texParameteri(Target, Var, Value); }
Imports.glActiveTexture = function(Texture) { gl.activeTexture(Texture); }
Imports.glUniform1i = function(Location, Value) { gl.uniform1i(GLObjects[Location], Value); }
Imports.glUniform3f = function(Location, x,y,z) { gl.uniform3f(GLObjects[Location], x,y,z); }
Imports.glCreateFramebuffer = function() {return PushGLObj(gl.createFramebuffer());}
Imports.glBindFramebuffer = function(Target, FB) {return gl.bindFramebuffer(Target, GLObjects[FB]);}
Imports.glFramebufferTexture2D = function(Target, Attachment, TexTarget, Texture, MipLevel) {gl.framebufferTexture2D(Target, Attachment, TexTarget, GLObjects[Texture], MipLevel);}
Imports.glBlitFramebuffer = function(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter) { gl.blitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);}

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
        const Data = new Uint8Array(WasmMemory.buffer, DataPtr, Size);
        gl.bufferData(Target, Data, UsageHint); // Allocate&upload to GPU
    }
    else
    {
        gl.bufferData(Target, Size, UsageHint); // Only allocate
    }
}

Imports.glBufferSubData = function(Target, Offset, Size, DataPtr)
{
    const Data = new Uint8Array(WasmMemory.buffer, DataPtr, Size);
    gl.bufferSubData(Target, Offset, Data);
}

Imports.glTexImage2D = function(Target, Level, InternalFormat, Width, Height, Border, Format, Type, DataPtr, DataSize, Offset)
{
    if(DataPtr && DataSize)
    {
        let Data;
        if(Type == gl.UNSIGNED_BYTE)
        {
            Data = new Uint8Array(WasmMemory.buffer, DataPtr, DataSize);
        }
        else if(Type == gl.FLOAT)
        {
            Data = new Float32Array(WasmMemory.buffer, DataPtr, DataSize/4);
        }

        gl.texImage2D(Target, Level, InternalFormat, Width, Height, Border, Format, Type, Data, Offset);
    }
    else
    {
        gl.texImage2D(Target, Level, InternalFormat, Width, Height, Border, Format, Type, null);
    }
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

    new Uint8Array(WasmMemory.buffer, PixelsPtr, PixelsSize).set(new Uint8Array(FontAtlasPixels));
    new Uint8Array(WasmMemory.buffer, GeomPtr, GeomSize).set(new Uint8Array(FontAtlasGeom));

    FontAtlasPixels = null;
    FontAtlasGeom = null;
}

Imports.JS_GetBusMeshSize = function()
{
    return BusMesh.byteLength;
}
Imports.JS_GetBusMesh = function(Ptr)
{
    new Uint8Array(WasmMemory.buffer, Ptr, BusMesh.byteLength).set(new Uint8Array(BusMesh));
}

Imports.JS_GL_LoadSkyboxFacesToBoundTexture = function()
{
    for(let i = 0; i < SkyboxFaces.length; ++i)
    {
        const Data = new Uint8Array(SkyboxFaces[i]);
        SkyboxFaces[i] = null;
        // TODO(robin): Don't hardcore these
        const Width = 1024;
        const Height = 1024;
        if(Data.byteLength != Width*Height*4)
        {
            throw Error("Unexpected image dimensions");
        }
        gl.texImage2D(gl.TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, gl.RGBA, Width, Height, 0, gl.RGBA, gl.UNSIGNED_BYTE, Data, 0);
    }

    SkyboxFaces = null;
}

async function Init()
{
    Program = WebAssembly.instantiateStreaming(Program, {"env":Imports});

    SkyboxFaces = await Promise.all(SkyboxFaces);

    FontAtlasGeom = await FontAtlasGeom;
    FontAtlasPixels = await FontAtlasPixels;
    BusMesh = await BusMesh;
    Program = await Program;

    WasmExports = Program.instance.exports;
    WasmMemory = WasmExports.memory;
    console.log(WasmExports);
    WasmExports.Init();

    function ResizeHandler()
    {
        gl.canvas.width = window.innerWidth;
        gl.canvas.height = window.innerHeight;
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
        if(Event.button == 0)
        {
            WasmExports.MouseLeft(true);
        }
        else if(Event.button == 1)
        {
            // TODO Middle mouse
        }
    });

    gl.canvas.addEventListener("mouseup", Event =>
    {
        if(Event.button == 0)
        {
            WasmExports.MouseLeft(false);
        }
        else if(Event.button == 1)
        {
            // TODO Middle mouse
        }
    });

    gl.canvas.addEventListener("mouseleave", Event =>
    {
        WasmExports.MouseLeave();
    });

    gl.canvas.addEventListener("wheel", Event =>
    {
        WasmExports.MouseWheel(Event.deltaY);
    });

    window.addEventListener("keydown", Event =>
    {
        WasmExports.KeyPress(Event.keyCode, true);
    });

    window.addEventListener("keyup", Event =>
    {
        WasmExports.KeyPress(Event.keyCode, false);
    });


    /*
    firebase.firestore().collection('blocks').onSnapshot(function(snapshot)
    {
        snapshot.docChanges().forEach(function(change)
        {
            const block = change.doc.data();
            console.log(change);

            const Str = block.name + '\n' + block.message;
            const StrPtr = AddJSStringToWasm(Str);
            const Color = Number.isInteger(block.color) ? block.color : 0xFF888888;
            if(change.type === "added")
            {
                WasmExports.AddDataBox(Str.length, StrPtr, Color);
            }
            else if(change.type === "modified")
            {
                WasmExports.UpdateDataBox(change.doc.newIndex, Str.length, StrPtr, Color);
            }
        });
    });
    */

    /*
    let ref = firebase.database().ref('blocks');
    ref.on('child_added', snapshot =>
    {
    });
    */

    let TestDataIndex = 0;
    let PrevTime = null;
    function RenderLoop(Now)
    {
        window.requestAnimationFrame(RenderLoop);

        /*
        if(TestData)
        {
            for(let i = 0; i < 128; ++i)
            {
                if(TestDataIndex < TestData.length/128)
                {
                    const Data = TestData[TestDataIndex++];

                    const Str = Data.name + '\n' + Data.email + '\n';
                    const Name = AddJSStringToWasm(Str);
                }
            }
        }
        */
        
        const DeltaTime = PrevTime ? (Now - PrevTime) : (1000.0/60.0);
        PrevTime = Now;

        WasmExports.UpdateAndRender(gl.canvas.width, gl.canvas.height, DeltaTime*0.001);
    }

    ResizeHandler();
    window.requestAnimationFrame(RenderLoop);
}

Init();


