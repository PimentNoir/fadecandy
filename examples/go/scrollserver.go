package main

import (
    "fmt"
    "net/http"
    "io/ioutil"
    "log"
    "flag"
    "time"
    "math/rand"
    "encoding/json"
    "github.com/kellydunn/go-opc"
)   

type Color struct {
    R, G, B uint8
}

type Scroller struct {
    delay, train_len int
    random bool
    color Color
}

var color = Color{255,0,0}
var home_c chan Scroller

func random(min, max int) uint8 {
    xr := rand.Intn(max - min) + min
    return uint8(xr)
}

func main() {
    rand.Seed(time.Now().Unix())
    
    serverPtr := flag.String("fcserver", "localhost:7890", "Fadecandy server and port to connect to")
    listenPortPtr := flag.Int("port", 8080, "Port to serve UI from")
    leds_len := flag.Int("leds", 40, "Number of LEDs in the string")
    flag.Parse()
    
    home_c = make(chan Scroller, 1)

    go func() { LEDSender(home_c, *serverPtr, *leds_len) }()
    
    fs := http.FileServer(http.Dir("./static"))
    http.Handle("/static/", http.StripPrefix("/static/", fs))
    http.Handle("/", http.StripPrefix("/", fs))
    http.HandleFunc("/update", UpdateHandler)

    log.Println("Listening on",fmt.Sprintf("http://0.0.0.0:%d",*listenPortPtr), "...")
    http.ListenAndServe(fmt.Sprintf(":%d",*listenPortPtr), nil)
}   

func UpdateHandler(w http.ResponseWriter, r *http.Request) {

    // do these stupid hacks for parsing JSON. 
    // go is pretty bad at this
    body, _ := ioutil.ReadAll(r.Body)    
    var f interface{}
    var inscroll Scroller
    json.Unmarshal(body, &f)

    m := f.(map[string]interface{})

    inscroll.delay = int(m["delay"].(float64))
    inscroll.train_len = int(m["train_len"].(float64))
    inscroll.random = bool(m["random"].(bool))
    colormap := m["color"].(map[string]interface{})
    inscroll.color.R = uint8(colormap["r"].(float64))
    inscroll.color.G = uint8(colormap["g"].(float64))
    inscroll.color.B = uint8(colormap["b"].(float64))

    ss := inscroll
    
    //send on the home channel, nonblocking
    select {
        case home_c <- ss:
        default:
            log.Println("msg NOT sent")
    }

    fmt.Fprintf(w, "HomeHandler", ss.delay)
} 

func LEDSender( c chan Scroller, server string, leds_len int ) {
        
    props := Scroller{40, 7, false, Color{255,0,0}}
    props.delay = 100

     // Create a client
    oc := opc.NewClient()
    err := oc.Connect("tcp", server)
    if err != nil {
        log.Fatal("Could not connect to Fadecandy server", err)
    }

    for { 
        for i := 0; i < leds_len; i++ {
            // send pixel data
            m := opc.NewMessage(0)
            m.SetLength(uint16(leds_len*3))
            
            for ii := 0; ii < props.train_len; ii++ {
                pix := i+ii
                if pix >= leds_len {
                    pix = props.train_len - ii - 1
                }
                if props.random {
                    m.SetPixelColor(pix, random(2,255), random(2,255), random(2,255))
                } else {
                    m.SetPixelColor(pix, props.color.R, props.color.G, props.color.B)
                }
            }
            
            err := oc.Send(m) 
            if err != nil {
                log.Println("couldn't send color",err)
            }             
            time.Sleep(time.Duration(props.delay) * time.Millisecond)

            // receive from channel
            select {
                case props = <- c:
                default:
            }
        }
    }
}
