package main

import "C"
import (
	"bytes"
	"encoding/binary"
	"encoding/json"
	"net/http"
	"os"
	"reflect"
	"unsafe"

	"github.com/lsds/KungFu/srcs/go/kungfu/execution"
	"github.com/lsds/KungFu/srcs/go/log"
	"github.com/lsds/KungFu/srcs/go/plan"
	"github.com/lsds/KungFu/srcs/go/rchannel/connection"
	"github.com/lsds/KungFu/srcs/go/utils"
)

var httpClient http.Client

func readConfigServer(url string) (*plan.Cluster, error) {
	f, err := utils.OpenURL(url, &httpClient, "")
	if err != nil {
		return nil, err
	}
	defer f.Close()
	var cluster plan.Cluster
	if err = json.NewDecoder(f).Decode(&cluster); err != nil {
		return nil, err
	}
	return &cluster, nil
}

func writeConfigServer(url string, cluster *plan.Cluster) error {
	buf := &bytes.Buffer{}
	if err := json.NewEncoder(buf).Encode(cluster); err != nil {
		return err
	}
	req, err := http.NewRequest(http.MethodPut, url, buf)
	if err != nil {
		return err
	}
	req.Header.Set("User-Agent", "stdml")
	resp, err := httpClient.Do(req)
	if err != nil {
		return err
	}
	resp.Body.Close()
	return nil
}

//export GoReadConfigServer
func GoReadConfigServer(ptr unsafe.Pointer, ptrSize int) int {
	cluster, err := readConfigServer(os.Getenv(`KUNGFU_CONFIG_SERVER`))
	if err != nil {
		return -1
	}
	*(*uint32)(unsafe.Pointer(uintptr(ptr) + 0)) = uint32(len(cluster.Runners))
	*(*uint32)(unsafe.Pointer(uintptr(ptr) + 4)) = uint32(len(cluster.Workers))
	out := boBytes(ptr, ptrSize)
	bs := cluster.Bytes()
	copy(out[8:8+len(bs)], bs)
	return len(bs) + 8
}

//export GoWriteConfigServer
func GoWriteConfigServer(ptr unsafe.Pointer, ptrSize int, newSize int) {
	cluster := parseCluster(ptr, ptrSize)
	log.Debugf("parsed cluster from c++: %s\n", cluster.DebugString())
	newCluster, err := cluster.Resize(newSize)
	if err == nil {
		writeConfigServer(os.Getenv(`KUNGFU_CONFIG_SERVER`), newCluster)
	}
}

type Stage struct {
	Version int
	Cluster plan.Cluster
}

func (s Stage) Encode() []byte {
	b := &bytes.Buffer{}
	json.NewEncoder(b).Encode(s)
	return b.Bytes()
}

//export GoCommitClusterConfig
func GoCommitClusterConfig(ptr unsafe.Pointer, ptrSize int, newVersion int) {
	cluster := parseCluster(ptr, ptrSize)
	stage := Stage{
		Version: newVersion,
		Cluster: cluster,
	}
	log.Debugf("state: %s", stage.Encode())
	var notify execution.PeerFunc = func(ctrl plan.PeerID) error {
		var self plan.PeerID
		conn := connection.New(ctrl, self, connection.ConnControl, 0, false)
		buf := stage.Encode()
		msg := connection.Message{
			Length: uint32(len(buf)),
			Data:   buf,
		}
		return conn.Send("update", msg, 0)
	}
	if err := notify.Par(cluster.Runners); err != nil {
		utils.ExitErr(err)
	}
}

func parseCluster(ptr unsafe.Pointer, ptrSize int) plan.Cluster {
	nr := *(*uint32)(unsafe.Pointer(uintptr(ptr) + 0))
	nw := *(*uint32)(unsafe.Pointer(uintptr(ptr) + 4))
	cluster := plan.Cluster{
		Runners: make(plan.PeerList, nr),
		Workers: make(plan.PeerList, nw),
	}
	out := boBytes(ptr, ptrSize)
	br := bytes.NewReader(out[8:])
	for i := range cluster.Runners {
		binary.Read(br, binary.LittleEndian, &cluster.Runners[i])
	}
	for i := range cluster.Workers {
		binary.Read(br, binary.LittleEndian, &cluster.Workers[i])
	}
	return cluster
}

func main() {}

func boBytes(ptr unsafe.Pointer, ptrSize int) []byte {
	sh := &reflect.SliceHeader{
		Data: uintptr(ptr),
		Len:  ptrSize,
		Cap:  ptrSize,
	}
	return *(*[]byte)(unsafe.Pointer(sh))
}
