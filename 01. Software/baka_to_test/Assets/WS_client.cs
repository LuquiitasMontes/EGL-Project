using UnityEngine;
using WebSocketSharp;
using TMPro;
using UnityEngine.XR;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System;

public class QuestWebSocketClient : MonoBehaviour
{
    private InputData _inputData;
    private WebSocket ws;
    public TextMeshProUGUI mytexto;
    public int udpPort = 8888; // El mismo que en el ESP32
    private bool websocketConnected = false;
    private string receivedIp = "";
    private string messageReceived = "";

    void Start()
    {
        _inputData = GetComponent<InputData>();
        mytexto.SetText("Buscando ESP32...");

        // Iniciar escucha UDP en hilo separado
        Thread udpListenerThread = new Thread(ListenForUdp);
        udpListenerThread.IsBackground = true;
        udpListenerThread.Start();
    }

    void ListenForUdp()
    {
        UdpClient udpClient = new UdpClient(udpPort);
        IPEndPoint remoteEP = new IPEndPoint(IPAddress.Any, udpPort);


        while (!String.Equals(messageReceived,"cart here"))
        {
            try
            {
                byte[] data = udpClient.Receive(ref remoteEP);
                messageReceived = Encoding.UTF8.GetString(data).Trim();
                receivedIp =  remoteEP.Address.ToString();

                Debug.Log("IP recibida por UDP: " + receivedIp);
                mytexto.text = "ESP IP: " + receivedIp;

                // Iniciar conexion con WebSocket
                if (String.Equals(messageReceived,"cart here"))
                	ConnectToWebSocket("ws://" + receivedIp + ":81");

            }
            catch (SocketException ex)
            {
                Debug.LogError("Error de UDP: " + ex.Message);
            }
        }

        udpClient.Close();
    }

    void ConnectToWebSocket(string url)
    {
        ws = new WebSocket(url);

        ws.OnOpen += (sender, e) =>
        {
            Debug.Log("Conectado al servidor WebSocket.");
            websocketConnected = true;
            ws.Send("car connected");
            mytexto.text = "Conectado al ESP32";
        };

        ws.OnError += (sender, e) =>
        {
            Debug.LogError("WebSocket error: " + e.Message);
        };

        ws.OnClose += (sender, e) =>
        {
            Debug.Log("Conexión WebSocket cerrada.");
            websocketConnected = false;
        };

        ws.ConnectAsync();
    }

    void Update()
    {
        // No me gusta mucho como quedo este control, voy a intentar mejorarlo despues de los finales
        // Cosas que arreglar:
        // 1. Enviar mensajes en flancos de cambio
        // 2. Dejar de enviar mensaje al recibir una confirmacion desde el esp32
        // 3. Tener en cuenta la sensibilidad del joystick
        // 4. Por alguna razon cuesta mas que registre los extremos cuando operas el carro
        if (!websocketConnected) return;

        bool isPressed_A;
        if (_inputData._rightController.TryGetFeatureValue(CommonUsages.primaryButton, out isPressed_A) && isPressed_A)
        {
            ws.Send("A");
            mytexto.SetText("A");
        }

        Vector2 joy_der;
        if (_inputData._rightController.TryGetFeatureValue(CommonUsages.primary2DAxis, out joy_der))
        {
            if (Mathf.Abs(joy_der.y) > 0.95f)
            {
                if (joy_der.y > 0)
                {
                    ws.Send("F");
                    mytexto.SetText("F");
                }
                else
                {
                    ws.Send("B");
                    mytexto.SetText("B");
                }
            }

            if (Mathf.Abs(joy_der.x) > 0.95f)
            {
                if (joy_der.x > 0)
                {
                    ws.Send("R");
                    mytexto.SetText("R");
                }
                else
                {
                    ws.Send("L");
                    mytexto.SetText("L");
                }
            }
        }
    }

    void OnApplicationQuit()
    {
        if (ws != null && ws.IsAlive)
        {
            ws.Close();
        }
    }
}
