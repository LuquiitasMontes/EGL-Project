using UnityEngine;
using WebSocketSharp;
using TMPro;
using UnityEngine.XR;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System;

public class WebSocketClientArm1 : MonoBehaviour
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


        while (!String.Equals(messageReceived,"arm1 here"))
        {
            try
            {
                byte[] data = udpClient.Receive(ref remoteEP);
                messageReceived = Encoding.UTF8.GetString(data).Trim();
                receivedIp =  remoteEP.Address.ToString();

                Debug.Log("IP recibida por UDP: " + receivedIp);
                mytexto.text = "ESP IP: " + receivedIp;

                // Iniciar conexion con WebSocket
                if (String.Equals(messageReceived,"arm1 here"))
                	ConnectToWebSocket("ws://" + receivedIp + ":82");

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
            ws.Send("arm1 connected");
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
        if (!websocketConnected) return;

        Vector3 pos;
        _inputData._rightController.TryGetFeatureValue(CommonUsages.devicePosition, out pos);
        ws.Send(pos[1].ToString());
        mytexto.SetText(pos[1].ToString());
	}


    void OnApplicationQuit()
    {
        if (ws != null && ws.IsAlive)
        {
            ws.Close();
        }
    }
}
